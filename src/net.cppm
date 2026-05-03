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
#include <openssl/lhash.h>
#include <sys/epoll.h>
export module net;
import std;
export template <size_t N = 1024>
class Buffer {
    char buf_[N]{};
    std::span<char> span_ = {buf_, N};
public:
    auto& span() {
        return span_;
    }
    auto size() {
        return span_.size();
    }
    operator std::string() {
        return std::string(span_.begin(), span_.end());
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

export class Socket {
    Address addr_{};
    int fd_{};
public:
    Socket() {
        fd_ = socket(AF_INET, SOCK_STREAM, 0);
    }
    explicit Socket(const int fd) : fd_(fd) {  }
    explicit Socket(const Address &addr) : addr_(addr) {
        fd_ = socket(AF_INET, SOCK_STREAM, 0);
    }
    auto connect() {
        return ::connect(fd_, addr_.socket_address(), addr_.size());
    }
    auto bind() {
        return ::bind(fd_, addr_.socket_address(), addr_.size());
    }
    auto listen() {
        return ::listen(fd_, 5);
    }
    auto accept() {
        auto fd = ::accept(fd_, nullptr, nullptr);
        if (fd >= 0)
            return Socket(fd);
        std::string error_string;

        switch (errno) {
        case EAGAIN: // EWOULDBLOCK == EAGAIN
            error_string = "Resource temporarily unavailable (EAGAIN/EWOULDBLOCK)";
            break;

        case EINTR:
            error_string = "System call interrupted by signal (EINTR)";
            break;

        case EBADF:
            error_string = "Invalid file descriptor (EBADF)";
            break;

        case ENOTSOCK:
            error_string = "File descriptor is not a socket (ENOTSOCK)";
            break;

        case EINVAL:
            error_string = "Invalid argument or socket not in listening state (EINVAL)";
            break;

        case EMFILE:
            error_string = "Process file descriptor limit reached (EMFILE)";
            break;

        case ENFILE:
            error_string = "System-wide file descriptor limit reached (ENFILE)";
            break;

        case ECONNABORTED:
            error_string = "Connection aborted before accept() (ECONNABORTED)";
            break;

        case EFAULT:
            error_string = "Invalid memory address passed to system call (EFAULT)";
            break;

        case ENOBUFS:
        case ENOMEM:
            error_string = "Insufficient kernel memory (ENOBUFS/ENOMEM)";
            break;

        case EPERM:
            error_string = "Operation not permitted (EPERM)";
            break;

        case EOPNOTSUPP:
            error_string = "Operation not supported on this socket (EOPNOTSUPP)";
            break;

        default:
            error_string = std::string("Unknown error: ") + std::strerror(errno);
            break;
        }

        throw std::runtime_error(error_string);


    }
    auto send(std::span<char> span) {
        std::cout << span.size() << std::endl;
        return ::send(fd_, span.data(), span.size(), 0);
    }

    auto recv(std::span<char> &&span) {
        int n = ::recv(fd_, span.data(), span.size(), 0);
        return n;
    }
    auto recv(std::span<char>& span) {
        int n = ::recv(fd_, span.data(), span.size(), 0);
        span = {span.data(), static_cast<size_t>(n)};
        return n;
    }
    // auto recv(char buf[]) {
    //     return ::recv(fd_, buf, sizeof(buf), 0);
    // }

    ~Socket() {
        ::close(fd_);
    }
};

export using ::ssize_t;