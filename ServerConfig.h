#pragma once
#include <string>
#include <filesystem>
#include <fstream>
#include "boost/asio/ip/address_v4.hpp"
#include "boost/algorithm/string.hpp"
#include "boost/system/error_code.hpp"
#include <map>
#define TRANSFER_CONFIG "transfer.info"
constexpr uint16_t CLIENT_NAME_LEN = 100;

typedef std::string string;
typedef boost::asio::ip::address_v4 address_v4;

enum ErrorReports{
    SOCKET_PARAMS_MISSING,
    SOCKET_PARAMS_FAILED,
    IP_ADDR_INVALID,
    PORT_INVALID,
    CLIENT_NAME_MISSING,
    CLIENT_NAME_TO_LONG,
    FILE_NAME_MISSING,
    FILE_DOES_NOT_EXISTS
};

struct TransferInfo{
    string address;
    string port;
    string name;
    string filePath;
    TransferInfo() { address.clear(); port.clear(); name.clear(); filePath.clear(); }
};

class ServerConfig {

private:


    static ServerConfig* instance;
    ServerConfig();
    ~ServerConfig() {};

    static bool initialize();

    static bool GetSocketParams(std::string& paramsLine);

    static bool GetClientName(string& paramsLine);

    static bool GetFileName(string& paramsLine);

    static TransferInfo m_transferInfo;

    static std::map<ErrorReports, string> Err;

    static std::stringstream m_lastError;
public:
    //ServerConfig(std::string& filepath);
    static ServerConfig* Instance();

    TransferInfo& GetTransferInfo();
    string GetLastError();

    friend std::stringstream & operator<<(std::stringstream& os, string log);

};

