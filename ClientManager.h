#pragma once
//////////////////////////////
//      ClientManager
//
//
/////////////////////////////

#include <iostream>
#include <functional>
#include "ConfigManager.h"
#include "ClientLogic.h"

class ClientManager {

public:
    ClientManager();
    void clientStop(const std::string& error) const;
    void Initialize();
    void Run();

private:

    ClientLogic* m_clientLogic = nullptr;

    bool m_registered = false;

    string GetFileName();

    std::function<void (std::string)> m_clientError;

    string GetClientName();

};

