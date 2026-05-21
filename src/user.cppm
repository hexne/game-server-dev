/********************************************************************************
* @Author : hexne
* @Date   : 2026/05/01 18:08:30
********************************************************************************/

module;
#include <openssl/ssl.h>
export module user;
import std;
import net;

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




export class User {
    int id_{};
    std::string name_{};
    std::string number_{};
    std::string create_time_{};
public:
    User() = default;
    User(std::string id, const std::string& name, const std::string& number, const std::string& create_time)
        : id_{std::move(std::stoi(id))}, name_{name}, number_{number}, create_time_{create_time} {  }

    int id() const {
        return id_;
    }
    auto name() const {
        return name_;
    }
    auto number() const {
        return number_;
    }
    auto create_time() const {
        return create_time_;
    }
};
