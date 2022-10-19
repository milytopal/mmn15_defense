#pragma once

#include <osrng.h>
#include <rsa.h>
#include <iostream>
#include <fstream>
#include <filesystem>

#include "ServerConfig.h"
class FileHandler {
public:
    FileHandler();
    ~FileHandler();


    bool Read();
    bool Write();

    /*returns file for transfer*/
    std::string& GetFileName();
    std::string& GetClientName();

private:
    /* path to file for transfer */
    std::string m_pathTofFile;

private:
    void initialize();



};
