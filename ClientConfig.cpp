//
// Created by Mily Topal on 17/10/2022.
//

#include "ClientConfig.h"

ClientConfig* ClientConfig::instance = nullptr;

ClientConfig* ClientConfig::Instance()
{
    if (instance == nullptr)
    {
        instance = new ClientConfig();
        if(!initialize())
        {
            delete instance;
            instance = nullptr;
            return nullptr;
        }
    }
    return instance;
}
ClientConfig::ClientConfig(){
    Err = {
            {ErrorReports::SOCKET_PARAMS_MISSING, "Socket parameters are missing "},
            {ErrorReports::SOCKET_PARAMS_FAILED, "Failed to find socket parameters "},
            {ErrorReports::IP_ADDR_INVALID,"Ip address invalid"},
            {ErrorReports::PORT_INVALID, "Port number is invalid"},
            {ErrorReports::CLIENT_NAME_MISSING,"Client name is missing "},
            {ErrorReports::CLIENT_NAME_TO_LONG,"client name exceeded 100 characters "},
            {ErrorReports::FILE_DOES_NOT_EXISTS, "Failed to find 'transfer.info' file"},
            {ErrorReports::FILE_NAME_MISSING,"File name Missing"}
    };
    m_clientInfo = ClientInfo();
}

bool ClientConfig::initialize() {
    if(!std::filesystem::exists(REGISTRATION_CONFIG)) {
        m_lastError << "Client is not registered ";
    }
    else{
            auto fileStream = new std::fstream;

            fileStream->open(REGISTRATION_CONFIG, std::fstream::in);
            if (fileStream->is_open()) {
                std::string line;
                try
                {
                    std::getline(*fileStream, line);
                    if (!GetClientName(line)) {
                        return false;
                    }
                    std::getline(*fileStream, line);
                    if (!GetRSAKey(line)) {
                        return false;
                    }
                }
                catch (...)
                {
                    return false;
                }
                fileStream->close();
            }

    }
    return true;
}

ClientInfo &ClientConfig::GetClientInfo() {
    return m_clientInfo;
}

bool ClientConfig::GetRSAKey(string &paramsLine) {
    return false;
}

bool ClientConfig::GetClientName(string &paramsLine) {
    return false;
}


string ClientConfig::GetLastError() {
    return m_lastError.str();
}

std::string &operator<<(std::string &os, string log) {
    os.clear();
    os.append(log);
    return os;
}