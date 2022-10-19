//
// Created by Mily Topal on 10/10/2022.
//
#pragma once
#include "TcpClientChannel.h"
#include <map>
#include <iostream>
#include <string>
#include "ServerIcd.h"
#include "FileHandler.h"
#include "RSAWrapper.h"

std::stringstream  m_lastError;

#define clearErr const std::stringstream clean; \
m_lastError.str("");             \
m_lastError.clear();             \
m_lastError.copyfmt(clean);      \

#define LogErr \
clearErr       \
m_lastError << __FILE__ << " " << __LINE__ << " "



class ClientLogic {
public:
    struct Client
    {
        ClientID     id;
        std::string   username;
        PublicKey    publicKey;
        bool          publicKeySet    = false;
        SymmetricKey symmetricKey;
        bool          symmetricKeySet = false;
    };

    ClientLogic();

    ~ClientLogic();

    std::string GetLastError();
    void SubscribeToChannelError();
    std::function<void (std::string err)> subscribeToChannelError;

private:
    std::map<MessageType, std::string> m_msgDescription = {
            {MSG_SYMMETRIC_KEY_REQUEST, "symmetric key request"},
            {MSG_SYMMETRIC_KEY_SEND,    "symmetric key"},
            {MSG_TEXT,                  "text message"},
            {MSG_FILE,                  "file"}
    };


    bool ParseServeInfo();
    Client              m_self;           // self symmetric key invalid.
    std::vector<Client> m_clients;
    FileHandler*        m_fileHandler;
    TcpClientChannel*      m_socketHandler;
    RSAWrapper*   m_rsaDecryptor;

    void SetUpChannel();

    void clearError();
};

