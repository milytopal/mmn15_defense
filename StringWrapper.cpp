#include "StringWrapper.h"


std::string StringWrapper::encode(const std::string& str)
{
    std::string encoded;
    CryptoPP::StringSource ss(str, true,
                              new CryptoPP::Base64Encoder(
                                      new CryptoPP::StringSink(encoded)
                              ) // Base64Encoder
    ); // StringSource

    return encoded;
}

std::string StringWrapper::decode(const std::string& str)
{
    std::string decoded;
    CryptoPP::StringSource ss(str, true,
                              new CryptoPP::Base64Decoder(
                                      new CryptoPP::StringSink(decoded)
                              ) // Base64Decoder
    ); // StringSource

    return decoded;
}


std::string StringWrapper::hex(const uint8_t* buffer, const size_t size)
{
    if (size == 0 || buffer == nullptr)
        return "";
    const std::string byteString(buffer, buffer + size);
    if (byteString.empty())
        return "";
    try
    {
        return boost::algorithm::hex(byteString);
    }
    catch (...)
    {
        return "";
    }
}

/**
 * Try to convert hex string to bytes string.
 * Return empty string upon failure.
 */
std::string StringWrapper::unhex(const std::string& hexString)
{
    if (hexString.empty())
        return "";
    try
    {
        return boost::algorithm::unhex(hexString);
    }
    catch (...)
    {
        return "";
    }
}
// normalize string according to operating system
std::string StringWrapper::normalizePath(const std::string& messyPath) {
    std::filesystem::path path(messyPath);
    std::filesystem::path canonicalPath = std::filesystem::weakly_canonical(path);
    std::string npath = canonicalPath.make_preferred().string();
    return npath;
}