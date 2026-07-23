/********************************************************************************
* @Author : hexne
* @Date   : 2026/07/23 00:46:34
********************************************************************************/

module;
#include <openssl/ssl.h>
export module hash256;
import std;

export std::string sha256(std::string_view input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(
        reinterpret_cast<const unsigned char*>(input.data()),
        input.size(),
        hash
    );

    static const char* lut = "0123456789abcdef";

    std::string out;
    out.resize(SHA256_DIGEST_LENGTH * 2);

    for (size_t i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        unsigned char c = hash[i];
        out[i * 2]     = lut[c >> 4];
        out[i * 2 + 1] = lut[c & 0x0F];
    }
    return out;
}