#pragma once

#include <string>

#include <modes.h>
#include <aes.h>
#include <filters.h>

#include <stdexcept>

class AESWrapper
{
public:
    static const unsigned int DEFAULT_KEYLENGTH = 16;
private:
    unsigned char _key[DEFAULT_KEYLENGTH];
    AESWrapper(const AESWrapper& aes);
public:
    static unsigned char* GenerateKey(unsigned char* buffer, unsigned int length);

    AESWrapper();
    AESWrapper(const unsigned char* key, unsigned int size);
    ~AESWrapper();

    const unsigned char* getKey() const;
    static CryptoPP::RandomNumberGenerator gener;

    std::string encrypt(const char* plain, unsigned int length);
    std::string decrypt(const char* cipher, unsigned int length);
};