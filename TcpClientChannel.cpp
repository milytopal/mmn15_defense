//
// Created by Mily Topal on 01/10/2022.
//

#include "TcpClientChannel.h"

TcpClientChannel::TcpClientChannel(std::string name, bool reconnect, std::string address,
                                   std::string port):
_name(std::move(name)), _reconnect(reconnect),
_address(std::move(address)), _port(std::move(port)),
_connected(false)
{

}

TcpClientChannel::~TcpClientChannel() {
    close();
}

void TcpClientChannel::close()
{

}