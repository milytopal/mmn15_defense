//
// Created by Mily Topal on 10/10/2022.
//

#include "ClientLogic.h"

ClientLogic::ClientLogic() {

    subscribeToChannelError = std::function<void (std::string err)>([this](std::string err){
        m_lastError << err ;
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
//    std::stringstream err;
//    if (!m_fileHandler->open(SERVER_INFO))
//    {
//        LogErr << "Couldn't open " << SERVER_INFO;
//        return false;
//    }
//    std::string info;
//    if (!m_fileHandler->readLine(info))
//    {
//        LogErr << "Couldn't read " << SERVER_INFO;
//        return false;
//    }
//    m_fileHandler->close();
//    CStringer::trim(info);
//    const auto pos = info.find(':');
//    if (pos == std::string::npos)
//    {
//        LogErr << SERVER_INFO << " has invalid format! missing separator ':'";
//        return false;
//    }
//    const auto address = info.substr(0, pos);
//    const auto port = info.substr(pos + 1);
//    if (!m_socketHandler->setSocketInfo(address, port))
//    {
//        LogErr << SERVER_INFO << " has invalid IP address or port!";
//        return false;
//    }
    return true;
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

}

bool ClientLogic::HandelRegistrationApproved()
{

}
// build PublicKeyMessage
bool ClientLogic::SendPublicKeyToServer(uint8_t* key, size_t size)
{

}

bool ClientLogic::HandelSymmetricKeyResponseFromServer()
{
    // build message



    m_WaitingForResponse = true;
    AddMessageToQueue();

}
void ClientLogic::AddMessageToQueue(uint8_t* buff, size_t size)
{
    //std::pair<uint8_t*, size_t> newMessage(buff,size);
    m_MessageQueue.emplace(std::pair(buff,size));
    SendNextMessage();


}
// build SendEncryptedFileMessage
bool ClientLogic::SendEncryptedFileToServer(uint8_t* file, size_t size)
{
    // fileHandler->GetEncryptedFile(uint8_t* withThisAesKey, std::string fileName)

}

bool ClientLogic::SendNextMessage()
{
    if(m_WaitingForResponse == false && m_socketHandler->isOpen())
    {
        std::pair<uint8_t *, size_t > messageToSend(nullptr, 0);
        bool isThereMessToSend = false;
        if(!m_MessageQueue.empty())
        {
            messageToSend = get(m_MessageQueue.front());
            //isThereMessToSend =
        }
    }
}
