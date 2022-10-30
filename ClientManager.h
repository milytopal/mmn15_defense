#pragma once
//////////////////////////////
//      ClientManager
//
//
/////////////////////////////

#include <iostream>
#include "ServerConfig.h"
#include "ClientLogic.h"

class ClientManager {

public:
    ClientManager();
    void clientStop(const std::string& error) const;


private:
    ClientLogic m_clientLogic;
    void Initialize();
    bool m_registered = false;

    string GetFileName();

    string GetClientName();
};

