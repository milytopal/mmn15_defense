//
// Created by Mily Topal on 10/10/2022.
//

#include "ClientLogic.h"

ClientLogic::ClientLogic() {

    subscribeToChannelError = std::function<void (std::string err)>([this](std::string err){
        LogErr << err ;
    });

    SetUpChannel();
}

void ClientLogic::SetUpChannel()
{
m_socketHandler = new TcpClientChannel();
m_socketHandler->setNewDataSignalCallBack(subscribeToChannelError);
}

ClientLogic::~ClientLogic() {
    delete m_socketHandler;
};



bool ClientLogic::ParseServeInfo()
{
    std::stringstream err;
    if (!m_fileHandler->open(SERVER_INFO))
    {
        LogErr << "Couldn't open " << SERVER_INFO;
        return false;
    }
    std::string info;
    if (!m_fileHandler->readLine(info))
    {
        LogErr << "Couldn't read " << SERVER_INFO;
        return false;
    }
    m_fileHandler->close();
    CStringer::trim(info);
    const auto pos = info.find(':');
    if (pos == std::string::npos)
    {
        LogErr << SERVER_INFO << " has invalid format! missing separator ':'";
        return false;
    }
    const auto address = info.substr(0, pos);
    const auto port = info.substr(pos + 1);
    if (!m_socketHandler->setSocketInfo(address, port))
    {
        LogErr << SERVER_INFO << " has invalid IP address or port!";
        return false;
    }
    return true;
}

void ClientLogic::SubscribeToChannelError()
{
    subscribeToChannelError = std::function<void (std::stringstream err)>([this](std::stringstream err){
            LogErr << "" << err.str() ;
    });
}

std::string ClientLogic::GetLastError() {
    return m_lastError.str();
}
