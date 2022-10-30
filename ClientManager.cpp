//
// Created by Mily Topal on 16/10/2022.
//

#include "ClientManager.h"


ClientManager::ClientManager() : m_registered(false)
{

}


std::string ClientManager::GetFileName() {
    return ServerConfig::Instance().GetTransferInfo().filePath;
}

void ClientManager::Initialize() {

    if (!ServerConfig::Instance().GetTransferInfo().name.empty())
    {
        //clientStop(m_clientLogic.getLastError());
    }
 //   m_registered = m_clientLogic.parseClientInfo();



}

std::string ClientManager::GetClientName() {
    return ServerConfig::Instance().GetTransferInfo().name;
}



/**
 * Print error and exit client.
 */
void ClientManager::clientStop(const std::string& error) const
{
    std::cout << "Fatal Error: " << error << std::endl << "Client will stop." << std::endl;
    //pause();
    exit(1);
}