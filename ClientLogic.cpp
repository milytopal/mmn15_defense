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

constexpr int NUM_OF_ATTEMPTS = 3;  // Number of attempts to resend a message

void ClientLogic::printQueue() // only for debug todo: remove later
{
    for(auto & it : m_MessageQueue)
    {
        auto header = reinterpret_cast<RequestHeader&>(it.first);
        std::cout << " Message header in Queue: " << header.code << " Length: " << it.second << std::endl;
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
m_id(ClientID(uuid)),
m_privateKey(privateKey),
m_isRegistered(isRegistered),
m_socketHandler(),
m_fileHandler()
{
    initialize();


}

// Constructor if Client isn't registered
ClientLogic::ClientLogic(std::string& name, bool isRegistered):
m_name(ClientName(name)),
m_isRegistered(isRegistered),
m_socketHandler(),
m_fileHandler()
{
   initialize();

}

void ClientLogic::initialize()
{
    DEBUG("Client Was Created");
    subscribeToChannelError = new std::function<void(std::string err)>([this](std::string err){
        m_lastError << err ;
    });


    m_HandlersMap = {
            {ResponseCode::REGISTRATION_SUCCEEDED, [this](uint8_t* data, size_t size) { ClientLogic::HandelRegistrationApproved(data, size); m_MessageQueue.pop_front();}},
            {ResponseCode::REGISTRATION_FAILED,    [this](uint8_t* data, size_t size) { ClientLogic::HandelRegistrationFailed(data, size); }},
            {ResponseCode::PUBLIC_KEY_RECEIVED,    [this](uint8_t* data, size_t size) { ClientLogic::HandelSymmetricKeyResponseFromServer(data, size); m_MessageQueue.pop_front();}},
            {ResponseCode::MSG_RECEIVED_CRC_VALID, [this](uint8_t* data, size_t size) { ClientLogic::HandelMessageReceivedWithCrc(data, size); m_MessageQueue.pop_front();}},
            {ResponseCode::MSG_RECEIVED,           [this](uint8_t* data, size_t size) { ClientLogic::EndSession(); m_MessageQueue.pop_front();}}
    };

    if(!m_isRegistered)
    {
        CreateClientsRegistrationFile();
    }
    SetUpChannel();
    BuildRsaKeySet();
}

void ClientLogic::SetUpChannel()
{
    m_socketHandler = new TcpClientChannel("clientChannel", true, ConfigManager::GetTransferInfo().address, ConfigManager::GetTransferInfo().port);
    if(m_socketHandler == nullptr)
        return;
    m_socketHandler->setNewDataSignalCallBack(*subscribeToChannelError);
}

ClientLogic::~ClientLogic() {
    m_socketHandler->Close();
    delete[] m_socketHandler;
};


void ClientLogic::CreateClientsRegistrationFile()
{


}

void ClientLogic::SubscribeToChannelError()
{
//    subscribeToChannelError = std::function<void (std::stringstream err)>([this](std::stringstream err){
//            LogErr << "" << err.str() ;
//    });
}

std::string ClientLogic::GetLastError() {
    return m_lastError.str();
}

// build Registration Message
bool ClientLogic::SendRegistrationRequest()
{
    auto request = RegistrationRequestMessage(m_name);  // server ignores id field
    AddMessageToQueue(reinterpret_cast<uint8_t*>(&request), sizeof(RegistrationRequestMessage));

}

void ClientLogic::HandelRegistrationApproved(uint8_t * data, size_t size)
{
    auto response = reinterpret_cast<RegistrationSucceededResponseMessage*>(data);
    SetClientId(ClientID(response->payload));
    // todo: handle saving uuid to file
    // todo: GenerateKey and send it
    //SendPublicKeyToServer();
}

// build PublicKeyMessage
bool ClientLogic::SendPublicKeyToServer(uint8_t* key, size_t size)
{
    // build message
    auto request = PublicKeyRequestMessage(m_id, m_name);
    request.payload.clientName = m_name;
    request.payload.clientsPublicKey = m_publicKey;


    m_WaitingForResponse = true;
    AddMessageToQueue(reinterpret_cast<uint8_t*>(&request), sizeof(request));
    return true;
}

void ClientLogic::BuildRsaKeySet()
{
    if(m_isRegistered)
    {
        if(m_privateKey.empty())
            m_lastError.operator<<(__func__).operator<<("Client Registered but private key is empty! ");
        try{
            RSAPrivateWrapper wrap(m_privateKey);
            m_publicKey = PublicKey(wrap.getPublicKey());
        }
        catch(std::exception& e){
            std::cout << "private key is corrapted: " << e.what() << std::endl;
            return;
        }
    }
    else
    {
        RSAPrivateWrapper wrap;
        m_publicKey = PublicKey(wrap.getPublicKey());
        m_privateKey = wrap.getPrivateKey();
        ConfigManager::SetClientsPrivateKey(m_privateKey);
        ///////// tmp!!!!!! todo: deleteeeeee
        boost::uuids::uuid u; // initialize uuid

        std::string s1 = to_string(u);

        ConfigManager::SetClientsUuid(s1);
    }
    //for debug only: todo: remove later

    printBuff( "public key buffer:" ,&m_publicKey.publicKey, sizeof(m_publicKey.publicKey));
    printBuff("private key buffer:" ,(char*)m_privateKey.c_str(), m_privateKey.length());
}

void ClientLogic::HandelSymmetricKeyResponseFromServer(uint8_t * data, size_t size)
{
    auto response = reinterpret_cast<PublicKeyResponseMessage*>(data);
    std::string tmpKey;
    tmpKey.copy((char*)&response->payload.symmetricKey, SYMMETRIC_KEY_SIZE); // todo: check if works
    RSAPrivateWrapper decryptor = RSAPrivateWrapper(m_privateKey);
    m_symmetricKey = decryptor.decrypt(tmpKey);
    printBuff("Symmetric key buffer: " ,&m_symmetricKey.symmetricKey, sizeof(m_symmetricKey.symmetricKey));
}


void ClientLogic::AddMessageToQueue(uint8_t* buff, size_t size)
{
    m_MessageQueue.emplace_back(RequestMsg(buff,size));
    m_numOfAttempts = NUM_OF_ATTEMPTS;
    SendNextMessage();


}

// build SendEncryptedFileMessage
bool ClientLogic::SendEncryptedFileToServer(uint8_t* file, size_t size)
{
    auto request = RequestToSendFileMessage(m_id);
    // fileHandler->GetEncryptedFile(uint8_t* withThisAesKey, std::string fileName)
    std::string fileTosend = ConfigManager::Instance().GetFileToSend();
    size_t fileSize;
    FileHandler fHandler(fileTosend);
    fileSize = fHandler.GetFileLength();
    m_fileBuffer = std::make_unique<uint8_t>(fileSize);
    if(m_fileBuffer == nullptr)
    {
        m_lastError << __func__ << "Failed to get file to send" ;
        return false;
    }
    fHandler.Read(m_fileBuffer.get(),fileSize,std::ios_base::binary);
    AESWrapper aesWrap(m_symmetricKey.symmetricKey, SYMMETRIC_KEY_SIZE);
    auto encrypted = aesWrap.encrypt(reinterpret_cast<char*>(m_fileBuffer.get()), fileSize);
    request.payload.contentSize = fileSize;
    memcpy(&request.payload.fileName, fileTosend.c_str(), fileTosend.length());
    request.header.payloadSize = sizeof(RequestHeader) + fileSize;
    request.payload.data = m_fileBuffer.get();
    m_WaitingForResponse = true;
    AddMessageToQueue(reinterpret_cast<uint8_t*>(&request),request.header.payloadSize);

}
bool ClientLogic::WaitForResponse(ResponseCode expectedResponse)
{

}


bool ClientLogic::SendNextMessage()
{
    if(!m_WaitingForResponse && m_socketHandler->isOpen())
    {
        RequestMsg messageToSend(nullptr, 0);
        bool isThereMessToSend = false;
        if(!m_MessageQueue.empty())
        {
            messageToSend = m_MessageQueue.front();

            m_numOfAttempts = 3;
            SendMessageToChannel(messageToSend.first, messageToSend.second);
            //isThereMessToSend =
        }
    }
}

bool ClientLogic::SendMessageToChannel(uint8_t* buff, size_t length) {
    if(m_socketHandler->isOpen())
    {
        return m_socketHandler->Write(buff, length);
    }
    m_lastError << "Socket closed unexpectedly";
    return false;
}

bool ClientLogic::SetClientId(ClientID id) {
    if(m_id.isEmpty())
    {
        m_id = id;
        return true;
    }
    else
    {
        m_lastError << "Id Already set";
        return false;
    }
}

void ClientLogic::HandelRegistrationFailed(uint8_t * data, size_t size) {
// retry to send the last message

if(m_numOfAttempts > 0)
{
    auto messageToSend = m_MessageQueue.front();
    m_numOfAttempts--;
    SendMessageToChannel(messageToSend.first, messageToSend.second);
} else{
    m_WaitingForResponse = false;
    m_MessageQueue.pop_front();           // Remove last message that failed
    m_lastError << "Failed to Send Last message reached max number of attempts";
}
}

void ClientLogic::HandelMessageReceivedWithCrc(uint8_t * data, size_t size) {

}

void ClientLogic::EndSession()
{

}

void ClientLogic::Run()
{
    auto buffer = new uint8_t[1024];
    size_t bytesRed = 0;
    ResponseCode* response;
    ResponseHeader* header;
    if(m_socketHandler->Open())
    {
        SendRegistrationRequest();

        m_socketHandler->Read(buffer,1024, bytesRed, ResponseCode::REGISTRATION_SUCCEEDED);
        header = reinterpret_cast<ResponseHeader*>(buffer);
        response = reinterpret_cast<ResponseCode*>(header->code);
        m_HandlersMap.at(*response)(buffer, bytesRed);
    }
}

void ClientLogic::SetErrorCallback(std::function<void (std::string errTopic)> callback) {
    if(callback != nullptr)
    {
        PublishError = callback;
    }
    else
    {
        PublishError = [](std::string errTopic){};
    }

}

std::stringstream& ClientLogic::operator<<(string& log) {
    static std::stringstream os;
    os.operator<<(__FILE__).operator<<(": ").operator<<(log.c_str());
    PublishError(os.str());
    return os;
}

