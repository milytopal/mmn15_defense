//
// Created by Mily Topal on 10/10/2022.
//
#include <functional>
#include "AESWrapper.h"
#define DEBUG(msg) { std::cout << __FILE_NAME__ << ":   " << msg << std::endl; }
#include "ClientLogic.h"

#include "boost/lexical_cast.hpp"
#include "boost/uuid/uuid.hpp"
#include "boost/uuid/uuid_io.hpp"
#include "boost/crc.hpp"

constexpr int NUM_OF_ATTEMPTS = 3;  // Number of attempts to resend a message
using namespace std;

void ClientLogic::printQueue() // only for debug todo: remove later
{
    for(auto & it : m_MessageQueue)
    {
        auto header = reinterpret_cast<RequestHeader&>(*it.data());
        std::cout << " Message header in Queue: " << header.code << " Length: " << it.size() << std::endl;
    }
}

void ClientLogic::printBuff(std::string buffname ,void* buff, size_t length)
{
    uint8_t * buffptr = reinterpret_cast<uint8_t *>(buff);
    std::stringstream os;
    os << buffname << std::endl;
    for(int i=0; i<length; ++i) {
        os << std::hex << (int) buffptr[i] << "|";
        if(i%20 == 0 && i>0)
            os << std::endl<< "|";
    }
    DEBUG(os.str());
}
// Constructor if Client Registered
ClientLogic::ClientLogic(std::string& name, std::string& uuid, std::string& privateKey, bool isRegistered):
        m_name(ClientName(name)),
        m_id(ClientID((uint8_t*)uuid.c_str())),
        m_privateKey(privateKey),
        m_isRegistered(isRegistered),
        m_socketHandler()
{
    initialize();
}

// Constructor if Client isn't registered
ClientLogic::ClientLogic(std::string& name, bool isRegistered):
        m_name(ClientName(name)),
        m_isRegistered(isRegistered),
        m_socketHandler()
{
    initialize();
}

void ClientLogic::initialize()
{
    DEBUG("Client Was Created");

    m_HandlersMap = {
            {ResponseCode::REGISTRATION_SUCCEEDED, [this](uint8_t* data, size_t size) { m_MessageQueue.pop_front(); ClientLogic::HandelRegistrationApproved(data, size); }},
            {ResponseCode::REGISTRATION_FAILED,    [this](uint8_t* data, size_t size) { ClientLogic::HandelRegistrationFailed(data, size); }},
            {ResponseCode::PUBLIC_KEY_RECEIVED,    [this](uint8_t* data, size_t size) { m_MessageQueue.pop_front(); ClientLogic::HandelSymmetricKeyResponseFromServer(data, size);}},
            {ResponseCode::MSG_RECEIVED_CRC_VALID, [this](uint8_t* data, size_t size) { m_MessageQueue.pop_front(); ClientLogic::HandelMessageReceivedWithCrc(data, size); }},
            {ResponseCode::MSG_RECEIVED,           [this](uint8_t* data, size_t size) { m_MessageQueue.pop_front(); ClientLogic::EndSession(false); }}
    };

    SetUpChannel();
    BuildRsaKeySet();
    m_fileToSend = ConfigManager::Instance().GetFileToSend();
}

void ClientLogic::SetUpChannel()
{
    m_socketHandler = new TcpClientChannel("clientChannel", true, ConfigManager::GetTransferInfo().address, ConfigManager::GetTransferInfo().port);
    if(m_socketHandler == nullptr)
        return;
}

ClientLogic::~ClientLogic() {
    m_socketHandler->Close();
    delete m_socketHandler;
};

// build Registration Message
bool ClientLogic::SendRegistrationRequest()
{
    RegistrationRequestMessage request(m_name);  // server ignores id field
    AddMessageToQueue(reinterpret_cast<uint8_t*>(&request), sizeof(request));
    return true;
}


void ClientLogic::HandelRegistrationApproved(uint8_t * data, size_t size)
{
    cout << "Received Registration Approved from Server" << endl;
    m_WaitingForResponse = false;
    auto response = reinterpret_cast<RegistrationSucceededResponseMessage*>(data);
    SetClientId(ClientID(response->payload));
    printBuff("uuid Received: ", &m_id.uuid, CLIENT_ID_SIZE);
    SendPublicKeyToServer();
}


// build PublicKeyMessage
bool ClientLogic::SendPublicKeyToServer()
{
    m_WaitingForResponse = false;
    // build message
    PublicKeyRequestMessage request(m_id, m_name);
    memcpy(&request.payload.clientName,&m_name.name,CLIENT_NAME_SIZE);
    memcpy(&request.payload.clientsPublicKey, &m_publicKey.publicKey, PUBLIC_KEY_SIZE);
    cout << "Public Key Sent" << endl;
    AddMessageToQueue(reinterpret_cast<uint8_t*>(&request), sizeof(request));
    return true;
}

void ClientLogic::BuildRsaKeySet()
{
    if(m_isRegistered)
    {
        if(m_privateKey.empty())
            cerr <<(__func__) <<(": Client Registered but private key is empty! ");
        try{
            RSAPrivateWrapper wrap(m_privateKey);
            m_publicKey = PublicKey(wrap.getPublicKey());
        }
        catch(std::exception& e){
            std::cerr << "private key is corrapted: " << e.what() << std::endl;
            return;
        }
    }
    else
    {
        RSAPrivateWrapper wrap;
        m_publicKey = PublicKey(wrap.getPublicKey());
        m_privateKey = wrap.getPrivateKey();
    }

}

void ClientLogic::HandelSymmetricKeyResponseFromServer(uint8_t * data, size_t size)
{
    cout << "Received Symmetric Key Response from Server" << endl;

    m_WaitingForResponse = false;
    m_dataSize = size;
    uint8_t *buff = new uint8_t[m_dataSize];
    memcpy (buff, data, size);
    auto response = reinterpret_cast<PublicKeyResponseMessage*>(buff);
    m_dataSize = (response->header.payloadSize) > CLIENT_ID_SIZE ? (response->header.payloadSize) - CLIENT_ID_SIZE : 0;
    std::stringstream key;
    for(int i= sizeof(ResponseHeader) + CLIENT_ID_SIZE ; i < size ;i++)
    {
        key << (char)buff[i];
    }
    RSAPrivateWrapper decryptor = RSAPrivateWrapper(m_privateKey);
    m_symmetricKey = decryptor.decrypt(key.str());
    cout << "Symmetric Key Recieved: " << key.str() << endl;
    SendEncryptedFileToServer();

}


void ClientLogic::AddMessageToQueue(uint8_t* buff, size_t size)
{
    if(buff == nullptr || size == 0)
        return;
    m_MessageQueue.emplace_back(RequestMsg(size));
    // copy content of buffer to the new vector created in the queue
    memcpy( m_MessageQueue.back().data(), buff,  size);
    m_numOfAttempts = NUM_OF_ATTEMPTS;
    SendNextMessage();
}

// build SendEncryptedFileMessage
bool ClientLogic::SendEncryptedFileToServer()
{
    m_WaitingForResponse = false;

    RequestToSendFileMessage request(m_id);

    // load file to send
    size_t fileSize;
    FileHandler fileHandler(m_fileToSend);
    fileSize = fileHandler.GetFileLengthWithPadding();
    uint8_t * tempbuff = new uint8_t[fileSize];

    memset(tempbuff, 0 , fileSize);
    fileHandler.ReadFileWithPadding(tempbuff, fileSize , std::ios_base::binary);

    // calculate crc at this step to avoid reading the file twice
    boost::crc_32_type crc;
    crc.process_bytes(tempbuff, fileSize);
    m_calculatedCrc = crc.checksum();

    // encrypting file
    AESWrapper aesWrap(reinterpret_cast<unsigned char*>(&m_symmetricKey.symmetricKey), SYMMETRIC_KEY_SIZE);

    auto encrypted = aesWrap.encrypt(reinterpret_cast<char*>(tempbuff), fileSize);

    request.payload.contentSize = fileSize;

    memcpy(&request.payload.fileName, m_fileToSend.c_str(), m_fileToSend.length());
    request.header.payloadSize = sizeof(uint32_t) + FILE_NAME_SIZE + encrypted.length();


    m_fileMsgBuffer = std::vector<uint8_t>();

    // packing message
    uint8_t* structIterator = reinterpret_cast<uint8_t*>(&request);

    for(int i=0 ; i< sizeof(RequestHeader)+FILE_NAME_SIZE+sizeof(int); i++)
    {
        m_fileMsgBuffer.push_back(*(structIterator + i));
    }
    for(auto &it : encrypted)
    {
        m_fileMsgBuffer.push_back(it);
    }

    cout << "File: " << m_fileToSend << " Sent to Server" << endl;
    // not using regular queue to avoid duplication - retry will be handled in the crc response handling
    SendMessageToChannel(m_fileMsgBuffer.data(),  m_fileMsgBuffer.size());

}


bool ClientLogic::SendNextMessage()
{
    if(!m_WaitingForResponse && m_socketHandler->isOpen())
    {
        if(!m_MessageQueue.empty())
        {
            auto messageToSend = m_MessageQueue.front();

            m_numOfAttempts = 3;
            SendMessageToChannel(messageToSend.data(), messageToSend.size());
        }
    }
}

bool ClientLogic::SendMessageToChannel(uint8_t* buff, size_t length) {
    if(m_socketHandler->isOpen() && buff != nullptr)
    {
        return m_socketHandler->Write(buff, length);
    }
    cerr << "Socket closed unexpectedly";
    return false;
}

bool ClientLogic::SetClientId(ClientID id) {
    if(m_id.isEmpty())
    {
        memcpy(&m_id.uuid, &id.uuid,CLIENT_ID_SIZE);
        ConfigManager::SetClientsUuid(m_id.to_string(m_id));
        ConfigManager::SetClientsPrivateKey(m_privateKey);
        return true;
    }
    else
    {
        cerr << "Id Already set";
        return false;
    }
}

void ClientLogic::HandelRegistrationFailed(uint8_t * data, size_t size) {
// retry to send the last message

    if(m_numOfAttempts > 0)
    {
        auto messageToSend = m_MessageQueue.front();
        m_numOfAttempts--;
        SendMessageToChannel(messageToSend.data(), messageToSend.size());
    } else{
        m_WaitingForResponse = false;
        m_MessageQueue.pop_front();           // Remove last message that failed
        cerr << "Failed to Send Last Message Reached Max Number of Attempts, Ending Session";
        EndSession(true);
    }
}

void ClientLogic::HandelMessageReceivedWithCrc(uint8_t * data, size_t size) {
    cout << "Received Crc Calculation from Server" << endl;
    m_WaitingForResponse = false;
    static int crcAttempts = 0;
    auto request = CrcNotValidResendRequestMessage(m_id);
    FileReceivedValidCrcResponseMessage validresponse;
    size_t messSize = 0;
    auto response = reinterpret_cast<FileReceivedValidCrcResponseMessage*>(data);
    memcpy(&request.payload.fileName, m_fileToSend.c_str(), m_fileToSend.length());

    std::cout << "Clients Calculated Crc: "<< m_calculatedCrc << "  Received From Server:" <<
              ( unsigned int)response->payload.checksum <<std::endl;

    if( response->payload.checksum == m_calculatedCrc)
    {
        auto request = CrcValidRequestMessage(m_id);
        memcpy(&request.payload.fileName, m_fileToSend.c_str(), m_fileToSend.length());
        cout << "Crc is Valid Response Was Sent to Server" << endl;
        SendMessageToChannel(reinterpret_cast<uint8_t*>(&request),sizeof (request));
        return;
    }


    while(crcAttempts < NUM_OF_ATTEMPTS)
    {
        if(SendMessageToChannel(reinterpret_cast<uint8_t*>(&request),sizeof (request)))
        {
            m_socketHandler->Read(reinterpret_cast<uint8_t*>(&validresponse), sizeof(validresponse), messSize, ResponseCode::MSG_RECEIVED_CRC_VALID);
            if(validresponse.header.code == ResponseCode::MSG_RECEIVED_CRC_VALID)
            {
                std::cout << "Clients Calculated Crc: "<< m_calculatedCrc << "  Received From Server:" << (int)validresponse.payload.checksum << endl;
                if(validresponse.payload.checksum == m_calculatedCrc)
                {
                    auto request = CrcValidRequestMessage(m_id);
                    memcpy(&request.payload.fileName, m_fileToSend.c_str(), m_fileToSend.length());
                    SendMessageToChannel(reinterpret_cast<uint8_t*>(&request),sizeof (request));
                    break;
                }
                else{
                    crcAttempts++;
                    cout << "Server calculated wrong crc, resend number: " << crcAttempts << endl;
                }
            }
            else
            {
                // handle other code
            }
        }
    }
    if(crcAttempts == NUM_OF_ATTEMPTS)      // exited loop because reached the 3 attempts
    {
        auto request = CrcNotValidAbortRequestMessage(m_id);
        memcpy(&request.payload.fileName, m_fileToSend.c_str(), m_fileToSend.length());
        SendMessageToChannel(reinterpret_cast<uint8_t*>(&request),sizeof (request));
        cerr << "Server failed to calculate Crc, closing clients operation, goodbye " << endl;
        EndSession(true);
    }
    // else: crc ok already sent
}

void ClientLogic::EndSession(bool error)
{
    m_WaitingForResponse = false;
    m_socketHandler->Close();
    if(!error)
        cout << "File transferred successfully Hurray!";
    else
        cerr << " ¯\\_(ツ)_/¯ " << endl;
    exit(0);
}

void ClientLogic::Run()
{
    auto buffer = new uint8_t[1024];
    size_t bytesRed = 0;
    int response;
    ResponseHeader header;
    if(m_socketHandler->Open())
    {
        if(!m_isRegistered)
            SendRegistrationRequest();
        else
            SendPublicKeyToServer();

        while(m_socketHandler->isOpen()){
            m_socketHandler->Read(buffer,1024, bytesRed, ResponseCode::REGISTRATION_SUCCEEDED);


            header = reinterpret_cast<ResponseHeader&>(*buffer);
            response = header.code;
            if(response >= ResponseCode::REGISTRATION_SUCCEEDED && response <= ResponseCode::MSG_RECEIVED){
                for (auto &it: m_HandlersMap) {
                    if (it.first == (response)) {
                        it.second(buffer, header.payloadSize + sizeof(ResponseHeader));
                        break;
                    }
                }
            }
        }
    }
    delete[] buffer;
}

