//
// Created by Mily Topal on 16/10/2022.
//

#include "ClientManager.h"


ClientManager::ClientManager() : m_registered(false)
{

}


std::string ClientManager::GetFileName() {
    return ConfigManager::GetTransferInfo().filePath;
}

void ClientManager::Initialize() {

    if (ConfigManager::Instance().GetTransferInfo().name.empty())
    {
        clientStop(ConfigManager::GetLastError());
    }
    if(ConfigManager::Instance().isRegistered())
    {
        auto info = ConfigManager::Instance().GetClientInfo();
        m_clientLogic = new ClientLogic(info.name,info.uuid, info.RSAPrivatekey, true);
    }
    else
    {
        auto info = ConfigManager::GetTransferInfo();
        m_clientLogic = new ClientLogic(info.name, false);
    }

}

void ClientManager::Run()
{
    m_clientLogic->Run();
}
/**
 * Print error and exit client.
 */
void ClientManager::clientStop(const std::string& error) const
{
    std::cout << "Fatal Error: " << error << std::endl << "Client will stop." << std::endl;
    exit(1);
}

ClientManager::~ClientManager()
{
    delete m_clientLogic;
}

