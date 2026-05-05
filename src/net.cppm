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
    Address(const std::string& ip, const int port) {
        addr_.sin_family = AF_INET;
        addr_.sin_port = htons(port);

        if (::inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr) <= 0) {
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

class Socket {
    Address addr_{};
    int fd_ = -1;

    void close() noexcept {
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
    }

public:
    Socket() {
        fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        if (fd_ < 0) {
            throw std::runtime_error(std::string("socket failed: ") + std::strerror(errno));
        }
    }
    explicit Socket(const int fd) : fd_(fd) {
        if (fd_ < 0) {
            throw std::invalid_argument("invalid socket fd");
        }
    }
    explicit Socket(const Address &addr) : addr_(addr) {
        fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        if (fd_ < 0) {
            throw std::runtime_error(std::string("socket failed: ") + std::strerror(errno));
        }
    }

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    Socket(Socket&& other) noexcept
        : addr_(other.addr_), fd_(std::exchange(other.fd_, -1)) {}

    Socket& operator=(Socket&& other) noexcept {
        if (this != &other) {
            close();
            addr_ = other.addr_;
            fd_ = std::exchange(other.fd_, -1);
        }
        return *this;
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
        return ::send(fd_, span.data(), span.size(), 0);
    }
    auto recv(std::span<char> span) {
        int n = ::recv(fd_, span.data(), span.size(), 0);
        return n;
    }
    ~Socket() {
        close();
    }
};

export class TCP {
    using length_type = std::uint32_t;

    Socket socket_;
    static constexpr length_type max_message_size_ = 64 * 1024;

    explicit TCP(Socket socket) : socket_(std::move(socket)) {}

    void write_exact(std::span<char> span) {
        while (!span.empty()) {
            auto n = socket_.send(span);
            if (n < 0) {
                if (errno == EINTR)
                    continue;
                throw std::runtime_error(std::string("send failed: ") + std::strerror(errno));
            }
            if (n == 0)
                throw std::runtime_error("send failed: connection closed");
            span = span.subspan(static_cast<std::size_t>(n));
        }
    }

    bool read_exact(std::span<char> span) {
        while (!span.empty()) {
            auto n = socket_.recv(span);
            if (n < 0) {
                if (errno == EINTR)
                    continue;
                throw std::runtime_error(std::string("recv failed: ") + std::strerror(errno));
            }
            if (n == 0)
                return false;
            span = span.subspan(static_cast<std::size_t>(n));
        }
        return true;
    }

public:

    TCP() = default;
    explicit TCP(Address addr) : socket_(addr) {}

    auto connect() {
        return socket_.connect();
    }

    auto bind() {
        return socket_.bind();
    }

    auto listen() {
        return socket_.listen();
    }

    auto accept() {
        return TCP(socket_.accept());
    }

    void send(std::span<char> msg) {
        if (msg.size() > max_message_size_)
            throw std::runtime_error("message too large");

        auto len = static_cast<length_type>(msg.size());
        auto len_span = std::span{reinterpret_cast<char*>(&len), sizeof(len)};
        write_exact(len_span);
        write_exact(msg);
    }

    std::span<char> recv(std::span<char> buf) {
        length_type net_len{};
        auto len_span = std::span{reinterpret_cast<char*>(&net_len), sizeof(net_len)};
        if (!read_exact(len_span))
            return {};

        auto len = net_len;
        if (len > max_message_size_)
            throw std::runtime_error("message too large");

        if (len > buf.size())
            throw std::runtime_error("receive buffer too small");

        auto msg = buf.first(static_cast<std::size_t>(len));
        if (!read_exact(msg))
            return {};
        return msg;
    }

};

export using ::ssize_t;
