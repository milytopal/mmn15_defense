//
// Created by Mily Topal on 01/10/2022.
//

#include "FileHandler.h"

FileHandler::FileHandler() : m_pathTofFile(nullptr) {
    m_pathTofFile = ServerConfig::Instance().GetTransferInfo().filePath;
}

FileHandler::~FileHandler() {

}

bool FileHandler::Read() {
    return false;
}

bool FileHandler::Write() {
    return false;
}


void FileHandler::initialize() {

}
