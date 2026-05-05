/********************************************************************************
* @Author : hexne
* @Date   : 2026/05/04 22:55:06
********************************************************************************/

module;
export module message;
import std;

/*
 * 000 000 : login
 * 000 001 : heart
 */

using header_type = std::uint32_t;
export namespace header {
    enum class type : header_type {
        login, heart
    };
    void write(char *buf, type type) {
        auto v = static_cast<header_type>(type);
        std::memcpy(buf, &v, sizeof(v));
    }
    type read(char *buf) {
        header_type v;
        std::memcpy(&v, buf, sizeof(v));
        return static_cast<type>(v);
    }

    consteval std::size_t header_size() {
        return sizeof (header_type);
    }
}


export namespace message {

    std::size_t write(char *buf, header::type type, std::string_view msg) {
        std::uint32_t v = static_cast<std::uint32_t>(type);
        std::memcpy(buf, &v, sizeof(v));
        std::memcpy(buf + sizeof(v), msg.data(), msg.size());
        return sizeof(v) + msg.size();
    }





}