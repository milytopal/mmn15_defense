//
// Created by Mily Topal on 16/10/2022.
//

#include "UserInterface.h"


UserInterface::UserInterface() : m_registered(false)
{

}


std::string& UserInterface::GetFileName() {
    return ServerConfig::Instance()->GetTransferInfo().filePath;
}

void UserInterface::Initialize() {

    if (!ServerConfig::Instance()->GetTransferInfo().name.empty())
    {
        clientStop(_clientLogic.getLastError());
    }
    _registered = _clientLogic.parseClientInfo();


}

std::string &UserInterface::GetClientName() {
    return ServerConfig::Instance()->GetTransferInfo().name;
}



/**
 * Print error and exit client.
 */
void UserInterface::clientStop(const std::string& error) const
{
    std::cout << "Fatal Error: " << error << std::endl << "Client will stop." << std::endl;
    //pause();
    exit(1);
}