/********************************************************************************
* @Author : hexne
* @Date   : 2026/05/01 18:08:30
********************************************************************************/

module;
#include <openssl/ssl.h>
#include <meta>
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




export enum class UserStatus {
    online,
    offline,
    matching,
    in_room,
    in_battle
};


std::string enum_to_string(UserStatus status) {
    template for (constexpr auto e : std::define_static_array(std::meta::enumerators_of(^^UserStatus))) {
        if ([:e:] == status) {
            return std::string(std::meta::display_string_of(e));
        }
    }
    return std::string("UnKnow Status");
}


export class User {
    int id_{};
    std::string name_{};
    std::string number_{};
    std::string create_time_{};
    UserStatus status_ = UserStatus::offline;

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
    void status(UserStatus status) {
        status_ = status;
    }
    std::string status() {
        std::string status = enum_to_string(status_);
        auto pos = status.rfind("::");
        if (pos == std::string_view::npos)
            return status;
        return status.substr(pos + 2);
    }
    auto create_time() const {
        return create_time_;
    }
};
