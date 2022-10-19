#pragma once
//////////////////////////////
//      UserInterfaceManager
//      this is the only class interfaces with the user
//  handles user input, and managing logic including prior configurations
/////////////////////////////

#include <iostream>
#include "ServerConfig.h"
#include "ClientLogic.h"

class UserInterface {

public:
    UserInterface();
    void clientStop(const std::string& error) const;


private:
    ClientLogic m_clientLogic;
    void Initialize();
    bool m_registered = false;

    string &GetFileName();

    string &GetClientName();
};

