#pragma once

#include <string>
#include <base64.h>
#include "boost/algorithm/hex.hpp"
#include "filesystem"


class StringWrapper
{
public:
    static std::string encode(const std::string& str);
    static std::string decode(const std::string& str);

    static std::string normalizePath(const std::string& messyPath);
    static std::string hex(const uint8_t* buffer, const size_t size);
    static std::string unhex(const std::string& hexString);

};
