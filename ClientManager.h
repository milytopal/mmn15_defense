#pragma once
//////////////////////////////
//      ClientManager
//
//
/////////////////////////////

#include <iostream>
#include <functional>
#include "ServerConfig.h"
#include "ClientLogic.h"

class ClientManager {

public:
    ClientManager();
    void clientStop(const std::string& error) const;


private:

    class ResponseTrigger
    {
    public:
        enum Triggers
        {

        };
        std::function<void>* m_ActionOnTrigger = nullptr;



    };


    ClientLogic m_clientLogic;
    void Initialize();
    void SetTriggerResponse();




    bool m_registered = false;

    string GetFileName();

    string GetClientName();
};

