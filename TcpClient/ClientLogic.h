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

typedef std::vector<uint8_t> RequestMsg;

class ClientLogic {
public:
///// ClientLogic ////////////////////////////////////
// managing the client operations with the server
// if a me.info file is found the client will be created with the
// previous data
/////////////////////////////////////////////////////////
    ClientLogic(std::string& name, bool isRegistered);
    ClientLogic(std::string& name, std::string& uuid, std::string& privateKey, bool isRegistered);
    ~ClientLogic();
    std::string GetLastError();

    void initialize();

    void Run();


protected:
    ////// SetClientId //////
    // stores the id given by server using the ConfigManager
    bool SetClientId(ClientID id);

    // reactor map reacts according to the ResponseCode received
    std::unordered_map<ResponseCode ,std::function<void(uint8_t* data, size_t size)>> m_HandlersMap;

private:
    // Clients Traits
    ClientName      m_name;
    ClientID        m_id;
    PublicKey       m_publicKey;
    SymmetricKey    m_symmetricKey;
    std::string     m_privateKey;
    std::string     m_fileToSend;
    bool            m_isRegistered = false;

    TcpClientChannel*      m_socketHandler;
    std::vector<uint8_t> m_fileMsgBuffer;

    size_t m_calculatedCrc = 0;


    std::deque<RequestMsg> m_MessageQueue;
    bool m_WaitingForResponse = false;
    void SetUpChannel();

    int m_numOfAttempts = 3;

    size_t m_dataSize = 0;
    ///// BuildRsaKeySet ////////////////////////////////////
    // the function creates the private and public rsa key set,
    // if the client isn't registered new set will be created,
    // otherwise the client will create the public key from the private
    // that was saved in the 'me.info' file
    /////////////////////////////////////////////////////////
    void BuildRsaKeySet();

    void printBuff(std::string buffname ,void* buff, size_t length);

    ///// AddMessageToQueue ////////////////////////////////////
    // adding the last message to queue, if the request will be answered
    // with fail the  clientLogic will try to resend the last message again
    // after 3 failures the client will stop
    /////////////////////////////////////////////////////////
    void AddMessageToQueue(uint8_t *buff, size_t size);

    ///+++++++++ Functions For Handling Responses from Server +++++++///

    ///// HandelRegistrationFailed ////////////////////////////////////
    // if number of attempts hasn't reech 3 times the function will retry to send
    // the last message which is stored in queue
    // if excited 3 times the client will stop
    //////////////////////////////////////////////////////////////////
    void HandelRegistrationFailed(uint8_t * data, size_t size);

    ///// HandelSymmetricKeyResponseFromServer ///////////////////////
    // receiving the symmetric key from server and storing it in the private member
    // upon success will continue to the next step: SendEncryptedFileToServer
    //////////////////////////////////////////////////////////////////
    void HandelSymmetricKeyResponseFromServer(uint8_t * data, size_t size);

    ///// HandelRegistrationApproved ///////////////////////
    // receiving uuid from server and storing it using the Config Manager
    // upon success will continue to the next step: SendPublicKeyToServer
    void HandelRegistrationApproved(uint8_t * data, size_t size);

    ///// HandelMessageReceivedWithCrc ///////////////////////
    // receiving the response from server with the calculated crc
    // the function will validate the sizes and file name (for originality)
    // comparing results and sending out the decision
    // in this case the function will handle the next request to server
    // on its own and retrying to send the file
    //////////////////////////////////////////////////////////////////
    void HandelMessageReceivedWithCrc(uint8_t * data, size_t size);

    /////// EndSession //////////////////////////////////////////////
    // ends the session with server, receives boolean for printing use only.
    /////////////////////////////////////////////////////////////////
    void EndSession(bool error);


//++++++++++++++ Functions for Requests Sending +++++++++++++/////

    ///// SendRegistrationRequest ///////////////////////
    // sending registration request to server
    // this function will be called only if the client isn't registered
    //////////////////////////////////////////////////////////////////
    bool SendRegistrationRequest();

    ///// SendPublicKeyToServer ///////////////////////
    // building the message to send out to server containing
    // the public key
    //////////////////////////////////////////////////////////////////
    bool SendPublicKeyToServer();

    ///// SendEncryptedFileToServer ///////////////////////
    // building the encrypted file message,
    // the function will encrypt the file using the symmetric key
    // given by the server, because the data of the file is unknown
    // the function uses a vector and builds the whole message buffer on its own
    // sending it directly to the server. because the vector is a member of the class
    // its data won't be destroyed and will be available for the retries if needed
    //////////////////////////////////////////////////////////////////
    bool SendEncryptedFileToServer();

    ///// SendMessageToChannel ///////////////////////
    // sending buffer to channel if channel is open
    bool SendMessageToChannel(uint8_t *buff, size_t length);

    ///// SendNextMessage ///////////////////////
    // sending next message from the queue to channel
    bool SendNextMessage();

    void printQueue();
};

