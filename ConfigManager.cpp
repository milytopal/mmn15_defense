
#include "ConfigManager.h"
#include "iostream"
#include "boost/uuid/uuid_io.hpp"
#include "boost/uuid/uuid.hpp"
#include "boost/uuid/string_generator.hpp"
#include "RSAWrapper.h"

// indexes for writing to me.info
constexpr unsigned int NAME_LINE_INDEX      = 0;
constexpr unsigned int UUID_LINE_INDEX      = 1;
constexpr unsigned int RSA_KEY_LINE_INDEX   = 2;

ConfigManager* ConfigManager::instance = nullptr;

ConfigManager::ConfigManager(){
    initialize();
}

/* Get params from file
 * */
bool ConfigManager::initialize() {

    if(std::filesystem::exists(TRANSFER_CONFIG)) {
        FileHandler fHandler(TRANSFER_CONFIG);
        int linenum = 0;
        std::string line;
        line = fHandler.GetLine(linenum);
        if (!GetSocketParams(line))             // parsing checked
            return false;

        line = fHandler.GetLine(++linenum);
        if (!GetClientName(line))
            return false;

        line = fHandler.GetLine(++linenum);
        if (!GetFileName(line))
            return false;
    }
    else
    {
        m_lastError << Err.at(ErrorReports::FAILED_TO_OPEN_FILE) << ": " << TRANSFER_CONFIG;
    }

    return true;
}

bool ConfigManager::GetClientName(std::string& nameLine) {
    if (nameLine.empty()) {
        m_lastError << Err.at(ErrorReports::CLIENT_NAME_MISSING);
        return false;
    }boost::algorithm::trim(nameLine);

    if(nameLine.length() >= CLIENT_MAX_NAME) {
        m_lastError << Err.at(ErrorReports::CLIENT_NAME_TO_LONG);
        return false;
    }
    m_transferInfo.name = nameLine;
    m_clientInfo.name = nameLine;
    return true;
}

void ConfigManager::SetClientsUuid(string uuid)
{
    m_clientInfo.uuid = uuid;
    uuid = StringWrapper::encode(uuid);
    FileHandler fHandler(REGISTRATION_CONFIG);
    fHandler.WriteAtLine(uuid, UUID_LINE_INDEX);
}

void ConfigManager::SetClientsPrivateKey(string key)
{
    m_clientInfo.RSAPrivatekey = key;
    auto encodedKey = StringWrapper::encode(key);
    FileHandler fHandler(REGISTRATION_CONFIG);
    fHandler.WriteAtLine(encodedKey, RSA_KEY_LINE_INDEX);

}

bool ConfigManager::GetFileName(std::string& fileLine) {
    if (fileLine.empty()) {
        m_lastError << Err.at(ErrorReports::FILE_NAME_MISSING);
        return false;
    }
    boost::algorithm::trim(fileLine);
    std::string filename = fileLine;
#ifdef WIN32
    filename = StringWrapper::normalizePath(line);
#endif

    if(!std::filesystem::exists(filename)) {
        m_lastError << Err.at(ErrorReports::FILE_DOES_NOT_EXISTS) << ": " << fileLine;
        return false;
    }
    m_transferInfo.filePath = filename;
    return true;
}

bool ConfigManager::GetSocketParams(std::string& paramsLine) {
    if (paramsLine.empty()) {
        m_lastError << __func__ << Err.at(ErrorReports::SOCKET_PARAMS_MISSING);
        return false;
    }
    else {
        boost::algorithm::trim(paramsLine);
        const auto pos = paramsLine.find(':');
        if (pos == std::string::npos) {
            m_lastError << __func__ << Err.at(ErrorReports::SOCKET_PARAMS_FAILED);
            m_transferInfo.address = nullptr;
            m_transferInfo.port = nullptr;
        }
        string address = paramsLine.substr(0, pos);
        string port = paramsLine.substr(pos + 1);
        try {
            /* check if port is a number */
            int p = std::stoi(port);
            if (p == 0) { //  port 0 is invalid
                m_lastError << __func__ << Err.at(ErrorReports::PORT_INVALID);
                return false;
            }
            m_transferInfo.port = port;

            /* checking if address is valid*/
            if (address != "localhost") {
                boost::system::error_code err;
                boost::asio::ip::make_address_v4(address, err);
                if (err.failed()) {
                    m_lastError << __func__ << Err.at(ErrorReports::IP_ADDR_INVALID);
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


void ConfigManager::CreateRegistrationFile() {
    std::fstream registFile = std::fstream(REGISTRATION_CONFIG, std::ios_base::out);

    if(registFile.good())
    {
        auto pathh = std::filesystem::absolute(REGISTRATION_CONFIG);
        std::cout <<"registartion file created at: "<< pathh.string() << std::endl;
    }

    FileHandler fHandler(REGISTRATION_CONFIG);
    if(!fHandler.WriteAtLine(m_transferInfo.name, NAME_LINE_INDEX)) // write name of client in me.info
    {
        std::cout << __func__ << "Handler Failed to Write to file" << std::endl;
    }
}

bool ConfigManager::isRegistered(){

    if(!std::filesystem::exists(REGISTRATION_CONFIG)) {
        CreateRegistrationFile();
        return false;
    }
    else{
        auto fileStream = new std::fstream;

        fileStream->open(REGISTRATION_CONFIG, std::fstream::in);
        if (fileStream->is_open()) {
            std::string line;
            try
            {
                std::getline(*fileStream, line);
                if (!CheckClientName(line)) {
                    return false;
                }
                std::getline(*fileStream, line);
                if(!CheckClientsID(line)) {
                    return false;
                }
                std::string key((std::istreambuf_iterator<char>(*fileStream)),
                                std::istreambuf_iterator<char>());
                size_t size = key.length();

                if (!GetRSAKey(key)) {
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


bool ConfigManager::GetRSAKey(string &line) {
    if (line.empty()) {
        m_lastError << __func__ << "Clients Private key is missing!";
        return false;
    }
    boost::algorithm::trim(line);
    std::string res = StringWrapper::decode(line);
    //if(line.length() != RSAPrivateWrapper::BITS/8)
    // std::cout << "key length: " << res.length() << "  expected: " << RSAPrivateWrapper::BITS/8 << std::endl;
    m_clientInfo.RSAPrivatekey = res;
    return true;        // todo:  for now suuming true
}

bool ConfigManager::CheckClientsID(string& line)
{
    if (line.empty()) {
        m_lastError << __func__ << "Clients id is missing ";
        return false;
    }
    boost::algorithm::trim(line);
    line = StringWrapper::decode(line);     // todo: test
    line = StringWrapper::unhex(line);

    if(line.length() != CLIENT_ID_SIZE)
    {
        std::cout <<  "uuid length corupted!! ";
        return false;
    }
    m_clientInfo.uuid = line;
    return true;
}

bool ConfigManager::CheckClientName(string &line) {
    if (line.empty()) {
        m_lastError << __func__ << Err.at(ErrorReports::CLIENT_NAME_MISSING);
        return false;
    }
    boost::algorithm::trim(line);
    if(line.length() > CLIENT_MAX_NAME) {
        m_lastError << __func__ << Err.at(ErrorReports::CLIENT_NAME_TOO_LONG);
        return false;
    }
    auto alnumpPtr(std::find_if(line.begin(), line.end(), (int(*)(int))std::isalpha));
    if(alnumpPtr.base() == nullptr)
    {
        return false;
    }
    m_clientInfo.name = line;
    return true;
}


std::stringstream ConfigManager::operator<<( string& log) {
    std::stringstream os;
    os.clear();
    os.operator<<(__FILE__).operator<<(": ").operator<<(log.c_str());
    return os;
}

