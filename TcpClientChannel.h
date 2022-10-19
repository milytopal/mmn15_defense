#pragma once
#include <string>
#include <boost/asio.hpp>
#include <cstdlib>
#include <ostream>
#include <thread>
#include <boost/asio/ip/tcp.hpp>
#include "ServerIcd.h"

using boost::asio::ip::tcp;
using boost::asio::io_context;

class TcpClientChannel {
public:
    TcpClientChannel(std::string name, bool reconnect, std::string address,
                     std::string port);
    TcpClientChannel();
    ~TcpClientChannel();

    void Close();
    void run();
    bool Open();
    bool isOpen();
    bool Write();
    void setNewDataSignalCallBack(std::function<void (std::string errTopic)> callback);

    std::function<void (std::string errTopic)> ReportErrorToClient;

private:
    /* read full response message from server into buffer */
    bool Read(uint8_t* const buffer, const size_t size, size_t& bytesRed, ResponseCode expectedCode);




private:
    std::string    m_name;
    std::string    m_address;
    std::string    m_port;
    bool           m_reconnect = false;
    io_context*    m_ioContext;
    tcp::resolver* m_resolver;
    tcp::socket*   m_socket;
    bool           m_bigEndian = false;
    bool           m_isOpen = false;  // indicates that socket has been open and connected.
    bool           m_exitThread = true;
    std::thread m_readingThread;

    std::stringstream log;
    size_t GetHeader(uint8_t *buffer, ResponseCode expectedCode);
    bool ReadPayload(uint8_t* const buffer ,const size_t size ,size_t& bytesRed);


};


