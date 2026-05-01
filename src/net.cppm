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

export struct Address {
    std::string ip{};
    int port{};
    bool auto_assign = true;

    Address() = default;

    Address(std::string_view addr, int port)
        : ip(addr), port(port), auto_assign(false) { }
};

export class Socket {
    int fd_{-1};
    Address addr_;
public:
    // 构造函数：直接包装已有 fd（比如 accept 返回的）
    explicit Socket(int fd) : fd_(fd) {
        if (fd_ < 0) {
            throw std::runtime_error("invalid fd");
        }
    }

    // 默认构造：自动分配，不 bind
    Socket() : Socket(Address{}) { }

    // 根据 Address 构造：可选 bind
    explicit Socket(const Address& addr) : addr_(addr) {
        fd_ = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (fd_ < 0) {
            throw std::runtime_error("socket() failed");
        }

        if (!addr_.auto_assign) {
            sockaddr_in sa{};
            sa.sin_family = AF_INET;
            sa.sin_port = htons(addr_.port);
            sa.sin_addr.s_addr = inet_addr(addr_.ip.c_str());

            if (::bind(fd_, reinterpret_cast<sockaddr*>(&sa), sizeof(sa)) < 0) {
                ::close(fd_);
                throw std::runtime_error("bind() failed");
            }
        }
    }

    // 禁止拷贝，允许移动
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    Socket(Socket&& other) noexcept : fd_(other.fd_), addr_(std::move(other.addr_)) {
        other.fd_ = -1;
    }
    Socket& operator=(Socket&& other) noexcept {
        if (this != &other) {
            ::close(fd_);
            fd_ = other.fd_;
            addr_ = std::move(other.addr_);
            other.fd_ = -1;
        }
        return *this;
    }

    ~Socket() {
        if (fd_ >= 0) {
            ::close(fd_);
        }
    }

    void listen(int backlog = SOMAXCONN) {
        if (::listen(fd_, backlog) < 0) {
            throw std::runtime_error("listen() failed");
        }
    }

    std::optional<Socket> accept() {
        sockaddr_in client{};
        socklen_t len = sizeof(client);
        int cfd = ::accept4(fd_, reinterpret_cast<sockaddr*>(&client), &len, SOCK_NONBLOCK);
        if (cfd < 0)
            return std::nullopt;
        return Socket(cfd); // 返回一个新的 Socket 对象
    }

    void connect(const Address& server) {
        sockaddr_in sa{};
        sa.sin_family = AF_INET;
        sa.sin_port = htons(server.port);
        sa.sin_addr.s_addr = inet_addr(server.ip.c_str());

        if (::connect(fd_, reinterpret_cast<sockaddr*>(&sa), sizeof(sa)) < 0) {
            if (errno != EINPROGRESS) {
                throw std::runtime_error("connect() failed");
            }
        }
    }

    ssize_t read(char* buf, size_t len) {
        ssize_t n = ::recv(fd_, buf, len, 0);
        if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            throw std::runtime_error(std::string("recv() failed: ") + strerror(errno));
        }
        return n;
    }


    auto write(std::string_view buf) {
        return write(buf.data(), buf.size());
    }
    ssize_t write(const char* buf, size_t len) {
        ssize_t n = ::send(fd_, buf, len, 0);
        if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            throw std::runtime_error(std::string("send() failed: ") + strerror(errno));
        }
        return n;
    }
};

class EpollLoop {
    int epoll_fd_{};
public:
    void add(int fd, uint32_t events, std::coroutine_handle<> handle) {

    }

    void run() {

    }

};
