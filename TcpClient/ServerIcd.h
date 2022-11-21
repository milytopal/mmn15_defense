//
// Created by Mily Topal on 10/10/2022.
//  ServerIcd.h described the protocol between the client and the server
//  provides structured messages with no padding using pragma pack(push, 1)
//
//

#pragma once
#include <cstdlib>
#include "StringWrapper.h"
#include "iostream"

// Constants. All sizes are in BYTES.
constexpr size_t    MAX_INCOMING_PAYLOAD_SIZE   = (16 + 4 + 255 + 4) /* payload of FileReceivedValidCrcResponseMessage */;
constexpr uint8_t   CLIENT_VERSION              = 2;
constexpr size_t    CLIENT_ID_SIZE              = 16;
constexpr size_t    CLIENT_NAME_SIZE            = 255;
constexpr size_t    PUBLIC_KEY_SIZE             = 160;  // defined in protocol. 1024 bits.
constexpr size_t    SYMMETRIC_KEY_SIZE          = 16;   // defined in protocol.  128 bits.
constexpr size_t    FILE_NAME_SIZE              = 255;
constexpr size_t    PACKET_SIZE                 = 1024;

enum RequestCode
{
    REQUEST_REGISTRATION    = 1100,
    REQUEST_PUBLIC_KEY      = 1101,
    REQUEST_TO_SEND_FILE    = 1102,
    REQUEST_CRC_VALID       = 1104,          // not really a request as much as status report to server
    INVALID_CRC_RESEND_REQUEST  = 1105,
    INVALID_CRC_ABORT       = 1106
};

enum ResponseCode
{
    REGISTRATION_SUCCEEDED     = 2100,
    REGISTRATION_FAILED         = 2101,
    PUBLIC_KEY_RECEIVED         = 2102,      // send out AES key payload size unkown ?
    MSG_RECEIVED_CRC_VALID      = 2103,
    MSG_RECEIVED                = 2104
};


#pragma pack(push, 1)

struct ClientID
{
    uint8_t uuid[CLIENT_ID_SIZE] = {0};

    bool operator==(const ClientID& otherID) const {
        for (size_t i = 0; i < CLIENT_ID_SIZE; ++i)
            if (uuid[i] != otherID.uuid[i])
                return false;
        return true;
    };

    bool operator!=(const ClientID& otherID) const {
        return !(*this == otherID);
    };

    bool isEmpty() {
        auto tmp = std::bitset<CLIENT_ID_SIZE>(uuid);
        return tmp.none();
    };

    ClientID() =default;
    explicit ClientID(uint8_t* buff){
        if(buff != nullptr)
        {
            memcpy((char*)uuid, buff, CLIENT_ID_SIZE);
        }
    };
    // returning a readable string
    inline std::string to_string(ClientID u)
    {
        auto str = StringWrapper::hex(u.uuid, CLIENT_ID_SIZE);
        return str;
    }
};

struct ClientName
{
    uint8_t name[CLIENT_NAME_SIZE] = {0};
    bool isEmpty() {
        auto tmp = std::bitset<CLIENT_NAME_SIZE>(name);
        return tmp.none();
    };
    ClientName()= default;;
    explicit ClientName(std::string& nameStr){ memcpy(name, nameStr.c_str(), std::min((nameStr.length()), CLIENT_NAME_SIZE));};
};

struct PublicKey
{
    uint8_t publicKey[PUBLIC_KEY_SIZE] = {0};
    bool isEmpty() {
        auto tmp = std::bitset<PUBLIC_KEY_SIZE>(publicKey);
        return tmp.none();
    };
    PublicKey() = default;
    explicit PublicKey(const std::string& key){
        memcpy(this->publicKey, key.c_str(),  std::min(key.length(), PUBLIC_KEY_SIZE));
    };

};

struct SymmetricKey
{
    uint8_t symmetricKey[SYMMETRIC_KEY_SIZE] = {0};        // size varies
    bool isEmpty() {
        auto tmp = std::bitset<SYMMETRIC_KEY_SIZE>(symmetricKey);
        return tmp.none();
    };

    SymmetricKey&operator=(SymmetricKey& key)
    {
        return *this;
    };
    SymmetricKey&operator=(const std::string key)
    {
        memcpy(this->symmetricKey, key.c_str(),  std::min((key.length()), SYMMETRIC_KEY_SIZE));
        return *this;
    };
};


struct FileName
{
    uint8_t name[FILE_NAME_SIZE] = {0};
};

struct CRCStatus{
    ClientID clientId;
    FileName fileName = {0};
    explicit CRCStatus(const ClientID& id) : clientId(id){}
};
///////////////////////// Message Headers ////////////////////////
struct RequestHeader
{
    ClientID clientId;
    uint8_t version;
    uint16_t code;
    uint32_t payloadSize;
    explicit RequestHeader(const uint16_t reqCode) : version(CLIENT_VERSION), code(reqCode), payloadSize(0) {};
    RequestHeader(const ClientID& id, const uint16_t reqCode) : clientId(id), version(CLIENT_VERSION), code(reqCode), payloadSize(0) {};
};

struct ResponseHeader
{
    uint8_t version;
    uint16_t   code;
    uint32_t  payloadSize;
    ResponseHeader() : version(0), code(0), payloadSize(0) {};
};

/////////////// RESPONSES FROM SERVER //////////////////

struct RegistrationSucceededResponseMessage
{
    ResponseHeader header;
    ClientID       payload;
};


struct RegistrationFailedResponseMessage
{
    ResponseHeader header;               // no payload
};

struct PublicKeyResponseMessage          // received public key from client and sends symetric key
{
    ResponseHeader header;
    struct
    {
        ClientID   clientId;
        SymmetricKey*  symmetricKey;      // symmetrick key size is unknown
    }payload;
};

struct FileReceivedValidCrcResponseMessage
{
    ResponseHeader header;
    struct
    {
        ClientID   clientId;
        uint32_t contentSize;
        FileName  fileName;
        uint32_t checksum;
    }payload;
};

struct ReceivingApprovedResponseMessage
{
    ResponseHeader header;
};

///////////////////////// REQUESTS TO SERVER /////////////////////////

struct RegistrationRequestMessage /* Server ignores ClientID feild in header*/
{
    RequestHeader header;
    struct
    {
        ClientName clientName;
    }payload;
    explicit RegistrationRequestMessage(const ClientName name) : header(REQUEST_REGISTRATION) {
        payload.clientName = name;
        header.payloadSize = sizeof (payload);}; // server ignores ClientID
};

struct PublicKeyRequestMessage
{
    RequestHeader header;
    struct
    {
        ClientName clientName;
        PublicKey  clientsPublicKey;
    }payload;

    PublicKeyRequestMessage(const ClientID& id, const ClientName name) : header(id, REQUEST_PUBLIC_KEY)
    {
        payload.clientName = name;
        header.payloadSize = sizeof (payload);
    };
};

struct RequestToSendFileMessage
{
    RequestHeader header;
    struct{
        uint32_t contentSize = 0;
        FileName fileName;
        uint8_t* data = nullptr;                  // unknown size of file
    }payload;
    explicit RequestToSendFileMessage(const ClientID& id) : header(id, REQUEST_TO_SEND_FILE) {};
};

struct CrcValidRequestMessage {
    RequestHeader header;
    CRCStatus payload;
    explicit CrcValidRequestMessage(const ClientID& id) : header(id, REQUEST_CRC_VALID),payload(id) {header.payloadSize = sizeof (payload);}
};

struct CrcNotValidResendRequestMessage {
    RequestHeader header;
    CRCStatus payload;
    explicit CrcNotValidResendRequestMessage(const ClientID& id) : header(id, INVALID_CRC_RESEND_REQUEST),payload(id) {header.payloadSize = sizeof (payload);}
};


struct CrcNotValidAbortRequestMessage {
    RequestHeader header;
    CRCStatus payload;
    explicit CrcNotValidAbortRequestMessage(const ClientID& id) : header(id, INVALID_CRC_ABORT) ,payload(id) {
        header.payloadSize = sizeof (payload);
    }
};






#pragma pack(pop)