#pragma once
#include <string>
#include <filesystem>
#include <fstream>
#include "boost/asio/ip/address_v4.hpp"
#include "boost/algorithm/string.hpp"
#include "boost/system/error_code.hpp"
#include <map>

#define REGISTRATION_CONFIG "me.info"
#define TRANSFER_CONFIG "transfer.info"
constexpr uint16_t CLIENT_MAX_NAME = 100;
typedef std::string string;

typedef boost::asio::ip::address_v4 address_v4;


struct ClientInfo{
    string name;
    string RSAkey;
    ClientInfo(): name(""), RSAkey(""){};
};

struct TransferInfo{
    string address;
    string port;
    string name;
    string filePath;
    TransferInfo() { address.clear(); port.clear(); name.clear(); filePath.clear(); }
};

class ServerConfig
{

public:

    enum ErrorReports{
        SOCKET_PARAMS_MISSING,
        SOCKET_PARAMS_FAILED,
        IP_ADDR_INVALID,
        PORT_INVALID,
        CLIENT_NAME_MISSING,
        CLIENT_NAME_TO_LONG,
        FILE_NAME_MISSING,
        FILE_DOES_NOT_EXISTS,
        CLIENT_REGISTRATION_INFO_INVALID,
        CLIENT_NAME_TOO_LONG,
        RSA_KEY_MISSING,
        RSA_KEY_INVALID
    };

    static ServerConfig& Instance()
    {
        static ServerConfig instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }

    inline static TransferInfo GetTransferInfo(){ return m_transferInfo;};
    inline static string GetLastError(){ return m_lastError.str(); };

    friend std::stringstream & operator<<(std::stringstream& os, string& log);

private:

    static ServerConfig* instance;
    ServerConfig();
    ~ServerConfig() {};

    static bool initialize();

    static bool GetSocketParams(std::string& paramsLine);

    static bool GetClientName(string& paramsLine);

    static bool GetFileName(string& paramsLine);


   // inline static bool GetClientName(string& paramsLine);

    static bool GetRSAKey(string& paramsLine);
    inline ClientInfo GetClientInfo(){ return m_clientInfo; };

    static TransferInfo m_transferInfo;
    static ClientInfo m_clientInfo;
    static std::map<ErrorReports, string> Err;
    static std::stringstream m_lastError;

    bool initializeClientInfo();


    bool isRegistered();

    bool CheckClientName(string &line);
};

inline std::map<ServerConfig::ErrorReports, string> ServerConfig::Err = {
        {ServerConfig::ErrorReports::SOCKET_PARAMS_MISSING, "Socket parameters are missing "},
        {ServerConfig::ErrorReports::SOCKET_PARAMS_FAILED, "Failed to find socket parameters "},
        {ServerConfig::ErrorReports::IP_ADDR_INVALID,"Ip address invalid"},
        {ServerConfig::ErrorReports::PORT_INVALID, "Port number is invalid"},
        {ServerConfig::ErrorReports::CLIENT_NAME_MISSING,"Client name is missing "},
        {ServerConfig::ErrorReports::CLIENT_NAME_TO_LONG,"client name exceeded 100 characters "},
        {ServerConfig::ErrorReports::FILE_DOES_NOT_EXISTS, "Failed to find 'transfer.info' file"},
        {ServerConfig::ErrorReports::FILE_NAME_MISSING,"File name Missing"},
        {ServerConfig::ErrorReports::CLIENT_REGISTRATION_INFO_INVALID, "Client Registration Info is Corrupted "}
};

inline std::stringstream ServerConfig::m_lastError("");
inline TransferInfo ServerConfig::m_transferInfo = TransferInfo();
inline ClientInfo ServerConfig::m_clientInfo = ClientInfo();