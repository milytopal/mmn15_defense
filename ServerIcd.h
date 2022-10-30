//
// Created by Mily Topal on 10/10/2022.
//  ServerIcd.h described the protocol between the client and the server
//  provides structured messages with no padding using pragma pack(push, 1)
//
//

#pragma once
#include <cstdlib>


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
    PUBLIC_KEY_RECEIVED         = 2102,      // send out AES key
    MSG_RECEIVED_CRC_VALID      = 2103,
    MSG_RECEIVED                = 2104
};

enum MessageType   //todo: check if needed
{
    MSG_SYMMETRIC_KEY_REQUEST = 1,
    MSG_SYMMETRIC_KEY_SEND    = 2,
    MSG_TEXT                  = 3,
    MSG_FILE                  = 4
};


#pragma pack(push, 1)

struct ClientID
{
    uint8_t uuid[CLIENT_ID_SIZE];
    ClientID() : uuid{ 0 } {}

    bool operator==(const ClientID& otherID) const {
        for (size_t i = 0; i < CLIENT_ID_SIZE; ++i)
            if (uuid[i] != otherID.uuid[i])
                return false;
        return true;
    }

    bool operator!=(const ClientID& otherID) const {
        return !(*this == otherID);
    }

};

struct ClientName
{
    uint8_t name[CLIENT_NAME_SIZE] = {0};
};

struct PublicKey
{
    uint8_t publicKey[PUBLIC_KEY_SIZE] = {0};
};

struct SymmetricKey
{
    uint8_t symmetricKey[SYMMETRIC_KEY_SIZE] = {0};        // size varies
};


struct FileName
{
    uint8_t name[FILE_NAME_SIZE] = {0};
};

struct CRCStatus{
    ClientID clientId;
    FileName fileName = {0};
    CRCStatus(const ClientID& id) : clientId(id){}
};
///////////////////////// Message Headers ////////////////////////
struct RequestHeader
{
    ClientID clientId;
    uint8_t version;
    uint16_t code;
    uint32_t payloadSize;
    RequestHeader(const uint16_t reqCode) : version(CLIENT_VERSION), code(reqCode), payloadSize(0) {}
    RequestHeader(const ClientID& id, const uint16_t reqCode) : clientId(id), version(CLIENT_VERSION), code(reqCode), payloadSize(0) {}
};

struct ResponseHeader
{
    uint8_t version;
    uint16_t   code;
    uint32_t  payloadSize;
    ResponseHeader() : version(0), code(0), payloadSize(0) {}
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
        SymmetricKey  symmetricKey;      // symmetrick key size is unknown
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
    RegistrationRequestMessage(const ClientName name) : header(REQUEST_REGISTRATION) { payload.clientName = name;} // server ignores ClientID
};

struct PublicKeyRequestMessage
{
    RequestHeader header;
    struct
    {
        ClientName clientName;
        PublicKey  ServerPublicKey;
    }payload;
    PublicKeyRequestMessage(const ClientID& id, const ClientName name) : header(id, REQUEST_TO_SEND_FILE)
    {payload.clientName = name;}
};

struct RequestToSendFileMessage
{
    RequestHeader header;
    struct{
        uint32_t contentSize = 0;
        FileName fileName;
        uint8_t* data = nullptr;                  // unknown size of file
    }payload;
    RequestToSendFileMessage(const ClientID& id) : header(id, REQUEST_TO_SEND_FILE) {}
};

struct CrcValidRequestMessage {
    RequestHeader header;
    CRCStatus payload;
    CrcValidRequestMessage(const ClientID& id) : header(id, REQUEST_CRC_VALID),payload(id) {}
};

struct CrcNotValidResendRequestMessage {
    RequestHeader header;
    CRCStatus payload;
    CrcNotValidResendRequestMessage(const ClientID& id) : header(id, INVALID_CRC_RESEND_REQUEST),payload(id) {}
};


struct CrcNotValidAbortRequestMessage {
    RequestHeader header;
    CRCStatus payload;
    CrcNotValidAbortRequestMessage(const ClientID& id) : header(id, INVALID_CRC_ABORT) ,payload(id) {}
};






#pragma pack(pop)