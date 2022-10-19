
#include "ServerConfig.h"
#include "boost/asio/ip/tcp.hpp"
#include "iostream"



ServerConfig* ServerConfig::instance = nullptr;

ServerConfig* ServerConfig::Instance()
{
    if (instance == nullptr)
    {
        instance = new ServerConfig();
        if(!initialize())
        {
            delete instance;
            instance = nullptr;
            return nullptr;
        }
    }
    return instance;
}
ServerConfig::ServerConfig(){
    m_transferInfo = TransferInfo();
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
}

/* Get params from file
 * */
bool ServerConfig::initialize() {

    if(std::filesystem::exists(TRANSFER_CONFIG)) {
        auto fileStream = new std::fstream;

        fileStream->open(TRANSFER_CONFIG, std::fstream::in);
        if (fileStream->is_open()) {
            std::string line;
            try
            {
            std::getline(*fileStream, line);
            if (!GetSocketParams(line)) {
                return false;
            }
            std::getline(*fileStream, line);
            if (!GetClientName(line)) {
                return false;
            }
            std::getline(*fileStream, line);
            if (!GetFileName(line)) {
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
bool ServerConfig::GetClientName(std::string& nameLine) {
    if (nameLine.empty()) {
        m_lastError << Err.at(ErrorReports::CLIENT_NAME_MISSING);
        return false;
    }boost::algorithm::trim(nameLine);

    if(nameLine.length() >= CLIENT_NAME_LEN) {
        m_lastError << Err.at(ErrorReports::CLIENT_NAME_TO_LONG);
        return false;
    }
    m_transferInfo.name = nameLine;
    return true;
}

bool ServerConfig::GetFileName(std::string& fileLine) {
    if (fileLine.empty()) {
        m_lastError << Err.at(ErrorReports::FILE_NAME_MISSING);
        return false;
    }
    boost::algorithm::trim(fileLine);
    if(!std::filesystem::exists(fileLine)) {
        m_lastError << Err.at(ErrorReports::FILE_DOES_NOT_EXISTS);
        return false;
    }
    m_transferInfo.filePath = fileLine;
    return true;
}


bool ServerConfig::GetSocketParams(std::string& paramsLine) {
    if (paramsLine.empty()) {
        m_lastError << Err.at(ErrorReports::SOCKET_PARAMS_MISSING);
        return false;
    }
    else {
        boost::algorithm::trim(paramsLine);
        const auto pos = paramsLine.find(':');
        if (pos == std::string::npos) {
            m_lastError << Err.at(ErrorReports::SOCKET_PARAMS_FAILED);
            m_transferInfo.address = nullptr;
            m_transferInfo.port = nullptr;
        }
        string address = paramsLine.substr(0, pos);
        string port = paramsLine.substr(pos + 1);
        try {
            /* check if port is a number */
            int p = std::stoi(port);
            if (p == 0) { //  port 0 is invalid
                m_lastError << Err.at(ErrorReports::PORT_INVALID);
                return false;
            }
            m_transferInfo.port = port;

            /* checking if address is valid*/
            if (address != "localhost") {
                boost::system::error_code err;
                boost::asio::ip::make_address_v4(address, err);
                if (err.failed()) {
                    m_lastError << Err.at(ErrorReports::IP_ADDR_INVALID);
                    return false;
                }
                m_transferInfo.address = address;
            }
        }
        catch (...)
        {
            return false;
        }
    }
    return true;
}

TransferInfo& ServerConfig::GetTransferInfo() {

    return m_transferInfo;
}

string ServerConfig::GetLastError() {
    return m_lastError.str();
}

std::string &operator<<(std::string &os, string log) {
    os.clear();
    os.append(log);
    return os;
}

