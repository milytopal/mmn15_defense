#pragma once
#include <string>
#include <filesystem>
#include <fstream>
#include "boost/asio/ip/address_v4.hpp"
#include "boost/algorithm/string.hpp"
#include "boost/system/error_code.hpp"
#include <map>
#include "ServerIcd.h"
#include "FileHandler.h"
#include "StringWrapper.h"


#define REGISTRATION_CONFIG "me.info"
#define TRANSFER_CONFIG "transfer.info"
constexpr uint16_t CLIENT_MAX_NAME = 100;
typedef std::string string;

typedef boost::asio::ip::address_v4 address_v4;


struct ClientInfo{
    string name;
    string uuid;
    string RSAPrivatekey;
    ClientInfo(): name(""), RSAPrivatekey(""), uuid(""){};
};

struct TransferInfo{
    string address;
    string port;
    string name;
    string filePath;
    TransferInfo() { address.clear(); port.clear(); name.clear(); filePath.clear(); }
};

class ConfigManager
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
        RSA_KEY_INVALID,
        FAILED_TO_OPEN_FILE
    };

    static ConfigManager& Instance()
    {
        static ConfigManager instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }


    inline static std::string& GetFileToSend(){return m_transferInfo.filePath; };
    inline static TransferInfo& GetTransferInfo(){ return m_transferInfo;};
    inline static string GetLastError(){ return m_lastError.str(); };
    static void SetClientsUuid(string uuid);
    static void SetClientsPrivateKey(string key);
    std::stringstream operator<<(string& log);
    inline static ClientInfo& GetClientInfo(){ return m_clientInfo; };
    static bool isRegistered();

private:

    static ConfigManager* instance;
    ConfigManager();
    ~ConfigManager() {};

    static bool initialize();

    static bool GetSocketParams(std::string& paramsLine);

    static bool GetClientName(string& paramsLine);

    static bool GetFileName(string& paramsLine);

    static void CreateRegistrationFile();

   // inline static bool GetClientName(string& paramsLine);

    static bool GetRSAKey(string& paramsLine);

    static TransferInfo m_transferInfo;
    static ClientInfo m_clientInfo;
    static std::map<ErrorReports, string> Err;
    static std::stringstream m_lastError;

    static bool CheckClientName(string &line);
    static bool CheckClientsID(string &line);

    //bool HandleRegistration();
};

inline std::map<ConfigManager::ErrorReports, string> ConfigManager::Err = {
        {ConfigManager::ErrorReports::SOCKET_PARAMS_MISSING, "Socket parameters are missing "},
        {ConfigManager::ErrorReports::SOCKET_PARAMS_FAILED, "Failed to find socket parameters "},
        {ConfigManager::ErrorReports::IP_ADDR_INVALID,"Ip address invalid"},
        {ConfigManager::ErrorReports::PORT_INVALID, "Port number is invalid"},
        {ConfigManager::ErrorReports::CLIENT_NAME_MISSING,"Client name is missing "},
        {ConfigManager::ErrorReports::CLIENT_NAME_TO_LONG,"client name exceeded 100 characters "},
        {ConfigManager::ErrorReports::FILE_DOES_NOT_EXISTS, "Failed to find file"},
        {ConfigManager::ErrorReports::FILE_NAME_MISSING,"File name Missing"},
        {ConfigManager::ErrorReports::CLIENT_REGISTRATION_INFO_INVALID, "Client Registration Info is Corrupted "},
        {ConfigManager::ErrorReports::FAILED_TO_OPEN_FILE, "Failed To Open File"}
};

inline std::stringstream ConfigManager::m_lastError("");
inline TransferInfo ConfigManager::m_transferInfo = TransferInfo();
inline ClientInfo ConfigManager::m_clientInfo = ClientInfo();

