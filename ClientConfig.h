//
// Created by Mily Topal on 17/10/2022.
//
#pragma once
#include <string>
#include <filesystem>
#include <fstream>
#include "boost/asio/ip/address_v4.hpp"
#include "boost/algorithm/string.hpp"
#include "boost/system/error_code.hpp"

#define REGISTRATION_CONFIG "me.info"

constexpr uint16_t CLIENT_NAME_LEN = 100;

typedef std::string string;

struct ClientInfo{
    string name;
    string RSAkey;
    ClientInfo() { name.clear(); RSAkey.clear(); }
};

class ClientConfig {
private:
    enum ErrorReports{
        CLIENT_CONFIG_MISSING,
        FILE_DOES_NOT_EXISTS
    };

    static ClientConfig* instance;
    ClientConfig();
    ~ClientConfig() {};


    static bool GetClientName(string& paramsLine);

    static bool GetRSAKey(string& paramsLine);

    static ClientInfo m_clientInfo;

    static std::stringstream m_lastError;

public:
    static ClientConfig* Instance();

    static bool initialize();

    ClientInfo& GetClientInfo();

    string GetLastError();

    friend std::stringstream & operator<<(std::stringstream& os, string log);

};
