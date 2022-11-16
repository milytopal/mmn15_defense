//
// Created by Mily Topal on 01/10/2022.
//

#include "FileHandler.h"
using namespace std;

FileHandler::FileHandler(std::string filePath) : m_pathTofFile(filePath) {
    if(std::filesystem::exists(m_pathTofFile))
    {
        initialize();
    }
    else
    {
        std::cerr << "File " << m_pathTofFile << "Is Missing" << std::endl;
    }
}

FileHandler::~FileHandler() {
    if(m_pFile && m_pFile->is_open())
        m_pFile->close();
    delete m_pFile;
}


bool FileHandler::WriteAtLine(std::string content, unsigned int lineNum, std::ios_base::openmode mode)
{
    if(m_pFile == nullptr)
    {
        std::cerr << "file is missing " << std::endl;
        return false;
    }
    std::ios_base::openmode openMode = std::ios_base::out | mode;
    std::vector<std::string> fileContentVec;
    std::string line;
    m_pFile->open(m_pathTofFile, std::ios_base::in);
    if(m_pFile->is_open())
    {
        // loop until eof to get line from file
        for(int i=0; *m_pFile && !m_pFile->eof() || i<= lineNum; ++i)
        {
            std::getline(*m_pFile, line);
            if(!line.empty())
                fileContentVec.push_back(line);
            if(lineNum == i)
                fileContentVec.push_back(content);
        }
        m_pFile->close();
       // fileContentVec.insert(fileContentVec.begin()+ lineNum,(content.c_str()));
        //m_pFile->seekp(m_pFile->tellp());
        m_pFile->open(m_pathTofFile, std::ios_base::out);
        for(auto& it: fileContentVec) // write back to file with the new line
        {
            m_pFile->write(it.c_str(), it.length());
            m_pFile->put('\n');
        }
        m_pFile->close();
        m_pFile->sync();
        return true;
    }
    else
    {
        std::cout << "Failed to Open File";
        return false;
    }
}


size_t FileHandler::GetFileLength()
{
    std::error_code err;
    if(!std::filesystem::exists(m_pathTofFile))
    {
        std::cout << "failed to find file"<< std::endl;
        return 0;
    }
    std::filesystem::file_size(std::filesystem::path(m_pathTofFile), err); // function wount throw exception if errr_code is passed
    if(!err.message().empty())
        std::cout << err.message();
    return std::filesystem::file_size(std::filesystem::path(m_pathTofFile), err); // function wount throw exception if errr_code is passed
}

/* Read length characters from file into buffer
 * mode is by default in, if passed binary it will be added to the openmode*/
bool FileHandler::Read(uint8_t* buffer, size_t length, std::ios_base::openmode mode) {
    std::ios_base::openmode openMode = std::ios_base::in | mode;
    if(std::filesystem::exists(m_pathTofFile))
    {
        m_pFile->open((char*)m_pathTofFile.c_str(), openMode);
        m_pFile->tellg();
        if(m_pFile->readsome((char*)buffer, length) != length)
        {
            std::cout << __FILE__ << __func__ << "failed to read file";
        }
    }
}

bool FileHandler::Write(std::string& content) {

    return false;
}


void FileHandler::initialize() {
    try {
        m_pFile = new std::fstream;
    }
    catch(std::exception& e)
    {
        std::cerr << e.what() <<std::endl;
    }
}

std::string FileHandler::GetLine(unsigned int lineNum) {

    if(m_pFile == nullptr)
    {
        std::cerr << "file is missing " << std::endl;
        return "";
    }
    m_pFile->open(m_pathTofFile, std::fstream::in);
    if (m_pFile->is_open()) {
        std::string line;
        std::vector<string> lines;
        try {
            m_pFile->seekg(std::ios_base::beg);
            for(int i=0; i <= lineNum; ++i)
            {
                std::getline(*m_pFile, line);
                lines.push_back(line);
            }

            m_pFile->close();
            return lines[lineNum];
        }
        catch(std::exception& e) {
            std::cout << __FILE__ << __func__ << e.what();
            return "";
        }
    }
    else{
        std::cout << "Failed To Open File";
        return "";
    }

}
