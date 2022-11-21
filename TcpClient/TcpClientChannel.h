#pragma once
#include <string>
#include <boost/asio.hpp>
#include <cstdlib>
#include <ostream>
#include <thread>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/system/system_error.hpp>
#include "ServerIcd.h"

using boost::asio::ip::tcp;
using boost::asio::io_context;
using boost::asio::deadline_timer;
using boost::lambda::bind;
using boost::lambda::var;
using boost::lambda::_1;

class TcpClientChannel {
public:
    TcpClientChannel(std::string name, bool reconnect, std::string address,
                     std::string port);
    ~TcpClientChannel();

    void Close();
    bool Open();
    bool isOpen();
    bool Write(uint8_t* buffer, size_t length);
    std::function<void (std::string errTopic)> ReportErrorToClient;
    /* read full response message from server into buffer */
    bool Read(uint8_t* const buffer, const size_t size, size_t& bytesRed, ResponseCode expectedCode);
    bool           m_isOpen = false;  // indicates that socket has been open and connected.

protected:
    void CheckDeadline();
    std::string    m_name;
    std::string    m_address;
    std::string    m_port;
    bool           m_reconnect = false;
    io_context*    m_ioContext;
    deadline_timer*  m_deadline = nullptr;
    tcp::resolver* m_resolver;
    tcp::socket*   m_socket;
    bool           m_bigEndian = false;
    std::thread m_readingThread;
    bool Error();

    std::stringstream log;
    /*GetHeader reads ResponseHeader from the buffer received by the socket,
     * compares OpCodes and the size of payload declared in the header, validates it,
     * and returns the size of the payload that is left to read
     * in protocol there is only one message that the payload size expected is zero*/
    size_t GetHeader(uint8_t *buffer, ResponseCode expectedCode);
};


