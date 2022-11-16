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
#include <map>
#include <queue>
#include <unordered_map>
#include "ConfigManager.h"

typedef std::pair<uint8_t *, size_t > RequestMsg;

class ClientLogic {
public:
    ClientLogic(std::string& name, bool isRegistered);
    ClientLogic(std::string& name, std::string& uuid, std::string& privateKey, bool isRegistered);
    ~ClientLogic();
    std::string GetLastError();
    void SubscribeToChannelError();

    void initialize();

    void Run();

    void SetErrorCallback(std::function<void(string)> callback);

    std::function<void (std::string err)>* subscribeToChannelError;
    std::function<void (std::string err)> PublishError;

protected:
    bool SetClientId(ClientID id);
    std::unordered_map<ResponseCode ,std::function<void(uint8_t* data, size_t size)>> m_HandlersMap;

private:
    // Clients Traits
    ClientName      m_name;
    ClientID        m_id;
    PublicKey       m_publicKey;
    SymmetricKey    m_symmetricKey;
    std::string     m_privateKey;
    bool            m_isRegistered = false;

    FileHandler* m_fileHandler;
    TcpClientChannel*      m_socketHandler;
    //RSAWrapper*   m_rsaDecryptor;
    std::unique_ptr<uint8_t> m_fileBuffer = nullptr;
    std::deque<RequestMsg> m_MessageQueue;
    bool m_WaitingForResponse = false;
    void SetUpChannel();
    std::stringstream  m_lastError;
    std::stringstream &operator<<(string& log);



    void BuildRsaKeySet();

    void CreateClientsRegistrationFile();

    void printBuff(std::string buffname ,void* buff, size_t length);

    void clearError();

    int m_numOfAttempts;

    void AddMessageToQueue(uint8_t *buff, size_t size);

    bool ParseServeInfo();

    /////// Function For Handling Responses from Server ///////
    void HandelRegistrationFailed(uint8_t * data, size_t size);

    void HandelSymmetricKeyResponseFromServer(uint8_t * data, size_t size);

    void HandelRegistrationApproved(uint8_t * data, size_t size);

    void HandelMessageReceivedWithCrc(uint8_t * data, size_t size);

    /* Handling ResponseCode 2104 */
    void EndSession();
    /////// Functions for Requests Sending //////////
    bool SendPublicKeyToServer(uint8_t *key, size_t size);

    bool SendRegistrationRequest();

    bool SendEncryptedFileToServer(uint8_t *file, size_t size);

    bool SendMessageToChannel(uint8_t *buff, size_t length);

    bool SendNextMessage();

    bool WaitForResponse(ResponseCode expectedResponse);

    void printQueue();
};

