#pragma once

#include <osrng.h>
#include <rsa.h>
#include <iostream>
#include <fstream>
#include <filesystem>


class FileHandler {
public:
    FileHandler(std::string filePath);
    ~FileHandler();
    bool Read(uint8_t* buffer, size_t length, std::ios_base::openmode mode = std::ios_base::in);
    bool Write(std::string& content);
    bool WriteAtLine(std::string content, unsigned int lineNum, std::ios_base::openmode mode = std::ios_base::out);
    std::string GetLine(unsigned int lineNum);
    size_t GetFileLength();
private:
    /* path to file for transfer */
    std::string m_pathTofFile;
    std::fstream* m_pFile = nullptr;

private:
    void initialize();


};
