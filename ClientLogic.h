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
#include <queue>
class ClientLogic {
public:
    struct Client
    {
        ClientID     id;
        std::string   username;
        PublicKey    publicKey;
        bool          publicKeySet    = false;  // added function in struct - can be removed
        SymmetricKey symmetricKey;
        bool          symmetricKeySet = false;  // added function in struct - can be removed
    };

    ClientLogic(std::string name);

    ~ClientLogic();
    ClientID m_id;
    std::string GetLastError();
    void SubscribeToChannelError();
    std::function<void (std::string err)> subscribeToChannelError;
protected:
    bool SetClientId(ClientID id);


private:
    std::map<MessageType, std::string> m_msgDescription = {
            {MSG_SYMMETRIC_KEY_REQUEST, "symmetric key request"},
            {MSG_SYMMETRIC_KEY_SEND,    "symmetric key"},
            {MSG_TEXT,                  "text message"},
            {MSG_FILE,                  "file"}
    };

    ClientName m_name;
    bool ParseServeInfo();
    Client              m_self;           // self symmetric key invalid.

    std::vector<Client> m_clients;
    FileHandler* m_fileHandler;
    TcpClientChannel*      m_socketHandler;
    //RSAWrapper*   m_rsaDecryptor;
    std::queue<std::pair<uint8_t*, size_t>> m_MessageQueue;
    bool m_WaitingForResponse = false;
    void SetUpChannel();
    std::stringstream  m_lastError;
    void clearError();
    static int m_numOfAttempts;

    void AddMessageToQueue(uint8_t *buff, size_t size);

    bool SendPublicKeyToServer(uint8_t *key, size_t size);

    bool HandelSymmetricKeyResponseFromServer();

    bool HandelRegistrationApproved();

    bool SendRegistrationRequest();

    bool SendEncryptedFileToServer(uint8_t *file, size_t size);

    bool SendMessageToChannel(uint8_t *buff, size_t length);

    bool SendNextMessage();

    bool WaitForResponse(ResponseCode expectedResponse);
};

