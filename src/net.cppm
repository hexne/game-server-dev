/********************************************************************************
* @Author : hexne
* @Date   : 2026/04/23 14:30:19
********************************************************************************/
module;
#include <arpa/inet.h>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <sys/epoll.h>
export module net;
import std;
export template <size_t N = 1024>
class Buffer {
    std::array<char, N> buf_{};
public:
    auto span() {
        return std::span { buf_.data(), buf_.size() };
    }
};

export class Address {
    sockaddr_in addr_{};
public:
    // 默认构造：0.0.0.0:0
    Address() {
        addr_.sin_family = AF_INET;
        addr_.sin_addr.s_addr = INADDR_ANY;
        addr_.sin_port = 0;
    }


    // 从 IP + 端口构造
    Address(const std::string_view ip, const int port) {
        std::memset(&addr_, 0, sizeof(addr_));   // ← 必须加

        addr_.sin_family = AF_INET;
        addr_.sin_port = htons(port);

        if (::inet_pton(AF_INET, ip.data(), &addr_.sin_addr) <= 0) {
            throw std::runtime_error("invalid IPv4 address");
        }
    }


    // 从 sockaddr_in 构造（用于 UDP recvfrom）
    explicit Address(const sockaddr_in& sa) : addr_(sa) {}

    // 返回端口（主机字节序）
    int port() const {
        return ntohs(addr_.sin_port);
    }

    // 返回 IP 字符串
    std::string ip() const {
        char buf[INET_ADDRSTRLEN];
        const char* p = ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
        return p ? std::string(p) : std::string{};
    }

    const sockaddr_in& as_sockaddr_in() const { return addr_; }
    sockaddr_in& as_sockaddr_in() { return addr_; }

    const sockaddr* socket_address() const {
        return reinterpret_cast<const sockaddr*>(&addr_);
    }
    sockaddr* socket_address() {
        return reinterpret_cast<sockaddr*>(&addr_);
    }

    socklen_t size() const { return sizeof(sockaddr_in); }
};


export using ::ssize_t;