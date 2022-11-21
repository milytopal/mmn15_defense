#pragma once

#include <osrng.h>
#include <rsa.h>
#include <iostream>
#include <fstream>
#include <filesystem>


class FileHandler {
public:
    explicit FileHandler(std::string filePath);
    FileHandler() = delete;
    FileHandler(FileHandler&) = delete;
    ~FileHandler();
    bool Read(uint8_t* buffer, size_t length, std::ios_base::openmode mode = std::ios_base::in);
    bool ReadFileWithPadding(uint8_t* buffer, size_t length, std::ios_base::openmode mode = std::ios_base::in);
    bool WriteAtLine(std::string content, unsigned int lineNum, std::ios_base::openmode mode = std::ios_base::out);
    std::string GetLine(unsigned int lineNum);
    size_t GetFileLength();
    size_t GetFileLengthWithPadding();

private:
    /* path to file for transfer */
    std::string m_pathTofFile;
    std::fstream* m_pFile = nullptr;

private:
    void initialize();


};
