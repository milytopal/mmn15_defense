//
// Created by Mily Topal on 01/10/2022.
//

#include <iostream>
#include "TcpClientChannel.h"

TcpClientChannel::TcpClientChannel(std::string name, bool reconnect, std::string address,
                                   std::string port):
        m_name(std::move(name)),
        m_address(address),
        m_port(port),
        m_isOpen(false)
{

    ReportErrorToClient.operator=([](std::string errTopic){});

}

TcpClientChannel::~TcpClientChannel() {
    Close();
    delete m_ioContext;
    delete m_resolver;
    delete m_socket;
    m_ioContext = nullptr;
    m_resolver  = nullptr;
    m_socket    = nullptr;
}

void TcpClientChannel::Close()
{
    m_exitThread = true;
    try
    {
        if (m_socket != nullptr)
            m_socket->close();
    }
    catch (...) {} // Do Nothing

    m_isOpen = false;


}

bool TcpClientChannel::Open() {
    m_exitThread = false;
    try {
        Close();
        m_ioContext = new io_context;
        m_resolver  = new tcp::resolver(*m_ioContext);
        m_socket    = new tcp::socket(*m_ioContext);

        boost::asio::connect(*m_socket, m_resolver->resolve(m_address, m_port, tcp::resolver::query::canonical_name));
        m_socket->non_blocking(false);  // blocking socket

//    if(m_socket->is_open())
//    {
//        m_isOpen = true;
//        std::thread(std::function<void()>([this]()
//              {
//                  run();
//              })).detach();
//
//    }
    return (m_isOpen = m_socket->is_open());
    }
    catch(std::exception& e)
    {
        std::cerr << "channel returned with an error: "<< e.what() << std::endl;
        //return false;
    }
}
/* Read:    called only by ClientLogic
 * reads full message into buffer
 * Receives: buffer - denstination buffer given by client, size - expected size determined by the message the client is expecting,
 * bytesRed: actual bytes red
 * Exceptions: when the Message expected is the symetrick key which size is unknown but not more than 16 bytes(128 bits)*/
bool TcpClientChannel::Read(uint8_t* const buffer, const size_t size ,size_t& bytesRed, ResponseCode expectedCode)
{
    size_t bytesToRead = 0;
    size_t payloadSizeLeft = 0;
    ResponseHeader header;
    uint8_t *buffPtr = buffer;
    uint8_t *tempBuffPtr = nullptr;

    uint8_t tempBuffer[PACKET_SIZE] = { 0 };
    boost::system::error_code err; // read() will not throw exception when error_code is passed as argument.
    bytesRed = read(*m_socket, boost::asio::buffer(tempBuffer, PACKET_SIZE), err); // todo: might need to change size to packet size
    if (bytesRed == 0)
        return false;     // Error. Failed receiving and shouldn't use buffer.
    payloadSizeLeft = GetHeader(tempBuffer, expectedCode);
    header = reinterpret_cast<ResponseHeader&>(tempBuffer);
    if(payloadSizeLeft == 0 && header.code != (int)ResponseCode::REGISTRATION_FAILED && header.code != (int)ResponseCode::MSG_RECEIVED )
    {
        // GetHeader returned an error
        return false;
    }

    // Header validated
    if(header.payloadSize == 0) {
        return true;    // no payload expected
    }
    bytesToRead = payloadSizeLeft;
    const size_t copySize = (bytesToRead > bytesRed) ? bytesRed : bytesToRead;  // in case bytes red less than bytes to read
    memcpy(buffPtr, tempBuffer, copySize);
    buffPtr += copySize;
    bytesToRead = (bytesToRead < copySize) ? 0 : (bytesToRead - copySize);  // unsigned protection.
    // continue another while for getting payload
    while(bytesToRead > 0 && payloadSizeLeft > 0)
    {
        if (bytesToRead > PACKET_SIZE)
            bytesToRead = PACKET_SIZE;
        bytesRed = read(*m_socket, boost::asio::buffer(tempBuffer, bytesToRead), err); // todo: might need to change size to packet size
        const size_t copySize = (bytesToRead > bytesRed) ? bytesRed : bytesToRead;  // in case bytes red less than bytes to read
        memcpy(buffPtr, tempBuffer, copySize);
        buffPtr += copySize;
        bytesToRead = (bytesToRead < copySize) ? 0 : (bytesToRead - copySize);  // unsigned protection.
        payloadSizeLeft -= bytesRed;
    }
    return true;
}

/* Gets the header from the readed buffer red by channel and validates it */
size_t TcpClientChannel::GetHeader(uint8_t* buffer, ResponseCode expectedCode) // todo: remove expected code
{
    ResponseHeader header;
    uint32_t sizeExpected = 0;
    boost::system::error_code err; // read() will not throw exception when error_code is passed as argument.
    header = reinterpret_cast<ResponseHeader&>(buffer);
    if (header.code < REGISTRATION_SUCCEEDED || header.code > MSG_RECEIVED) // code received is not a response code
    {
        ReportErrorToClient("Received Unexpected Code");
        return 0;
    }
    switch (header.code)
    {
        case ResponseCode::REGISTRATION_SUCCEEDED:
        {
            sizeExpected = sizeof(RegistrationSucceededResponseMessage) - sizeof(ResponseHeader);
            break;
        }
        case ResponseCode::REGISTRATION_FAILED:
        {
            sizeExpected = sizeof(RegistrationFailedResponseMessage) - sizeof(ResponseHeader); //expected 0
            break;
        }
        case ResponseCode::MSG_RECEIVED_CRC_VALID:
        {
            sizeExpected = sizeof(FileReceivedValidCrcResponseMessage) - sizeof(ResponseHeader);
            break;
        }
        case ResponseCode::MSG_RECEIVED:
        {
            sizeExpected = sizeof(ReceivingApprovedResponseMessage) - sizeof(ResponseHeader); //expected 0
            break;
        }
        case ResponseCode::PUBLIC_KEY_RECEIVED: {
            sizeExpected = sizeof(PublicKeyResponseMessage) - sizeof(ResponseHeader); //expected 0
            if (header.payloadSize > MAX_INCOMING_PAYLOAD_SIZE)
            {
                log << "Unexpected payload size " << header.payloadSize << ". Expected size was " << sizeExpected;
                ReportErrorToClient (log.str());
                return 0;
            }
            return true; // Symmetric key size varies
        }
        default:
        {
            ReportErrorToClient("Received Unknown Code");
            return 0;
        }
    }
    if (header.payloadSize != sizeExpected)
    {
        log << "Unexpected payload size " << header.payloadSize << ". Expected size was " << sizeExpected;
        ReportErrorToClient (log.str());
        return 0;
    }
    return header.payloadSize;
}


bool TcpClientChannel::Write(uint8_t* buffer, size_t length)
{
    if(m_socket == nullptr || !m_socket->is_open() || m_ioContext->stopped() || !m_isOpen)
        return false;
    size_t bytesWritten = 0;
    size_t bytesToWrite = length;
    uint8_t* buffPtr = buffer;
    while(bytesToWrite > 0)
    {

        boost::system::error_code errorCode;
        bytesWritten = write(*m_socket, boost::asio::buffer(buffPtr,PACKET_SIZE), errorCode);
        if(bytesWritten == 0)
            return false;
        buffPtr+=bytesWritten;
        bytesToWrite = (bytesToWrite < bytesWritten) ? 0 : (bytesToWrite - bytesWritten);
    }
    return true;
}


void TcpClientChannel::setNewDataSignalCallBack(std::function<void (std::string errTopic)> callback) {
    if(callback != nullptr)
    {
        ReportErrorToClient.operator=(callback);
    }
    else
    {
        ReportErrorToClient = [](std::string errTopic){};
    }

}
bool TcpClientChannel::isOpen() {
    return m_isOpen;
}

void TcpClientChannel::run() {
    uint8_t buffer[PACKET_SIZE];

    while(!m_exitThread)
    {

    }

}
