//
// Created by Mily Topal on 10/10/2022.
//
#pragma once
#include "TcpClientChannel.h"
#include <map>
#include "ServerIcd.h"

class ClientLogic {
public:
    ClientLogic();

    ~ClientLogic();



private:
    std::map<MessageType, std::string> m_msgDescription = {
            {MSG_SYMMETRIC_KEY_REQUEST, "symmetric key request"},
            {MSG_SYMMETRIC_KEY_SEND,    "symmetric key"},
            {MSG_TEXT,                  "text message"},
            {MSG_FILE,                  "file"}
    };



};

