#pragma once
#include <string>
#include <boost/asio.hpp>
#include <cstdlib>
#include <ostream>
#include <boost/asio/ip/tcp.hpp>

using boost::asio::ip::tcp;
using boost::asio::io_context;

class TcpClientChannel {
public:
    TcpClientChannel(std::string name, bool reconnect, std::string address,
                     std::string port);
    ~TcpClientChannel();

    void close();


private:
    std::string    _name;
    std::string    _address;
    std::string    _port;
    bool           _reconnect;
    io_context*    _ioContext;
    tcp::resolver* _resolver;
    tcp::socket*   _socket;
    bool           _bigEndian;
    bool           _connected;  // indicates that socket has been open and connected.

};


