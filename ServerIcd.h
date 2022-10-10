//
// Created by Mily Topal on 10/10/2022.
//  ServerIcd.h described the protocol between the client and the server
//  provides structured messages with no padding using pragma pack(push, 1)
//
//

#pragma once
#include <cstdlib>


// Constants. All sizes are in BYTES.
constexpr uint8_t   CLIENT_VERSION         = 2;
constexpr size_t    CLIENT_ID_SIZE         = 16;
constexpr size_t    CLIENT_NAME_SIZE       = 255;
constexpr size_t    PUBLIC_KEY_SIZE        = 160;  // defined in protocol. 1024 bits.
constexpr size_t    SYMMETRIC_KEY_SIZE     = 16;   // defined in protocol.  128 bits.

enum RequestOpCode
{
    REQUEST_REGISTRATION   = 1000,
    REQUEST_CLIENTS_LIST   = 1001,
    REQUEST_PUBLIC_KEY     = 1002,
    REQUEST_SEND_MSG       = 1003,
    REQUEST_PENDING_MSG    = 1004
};

enum ResponseOpCode
{
    RESPONSE_REGISTRATION  = 2000,
    RESPONSE_USERS         = 2001,
    RESPONSE_PUBLIC_KEY    = 2002,
    RESPONSE_MSG_SENT      = 2003,
    RESPONSE_PENDING_MSG   = 2004,
    RESPONSE_ERROR         = 9000
};

enum MessageType
{
    MSG_SYMMETRIC_KEY_REQUEST = 1,
    MSG_SYMMETRIC_KEY_SEND    = 2,
    MSG_TEXT                  = 3,
    MSG_FILE                  = 4
};


#pragma pack(push, 1)




#pragma pack(pop)