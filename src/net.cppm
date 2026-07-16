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

    void set_nonblocking() {
        int flags = fcntl(fd_, F_GETFL, 0);
        fcntl(fd_, F_SETFL, flags | O_NONBLOCK);
    }
    void set_blocking() {
        int flags = fcntl(fd_, F_GETFL, 0);
        fcntl(fd_, F_SETFL, flags & ~O_NONBLOCK);
    }

public:
    Socket() {
        fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        if (fd_ < 0) {
            throw std::runtime_error(std::string("socket failed: ") + std::strerror(errno));
        }
        set_nonblocking();
    }
    explicit Socket(const int fd) : fd_(fd) {
        if (fd_ < 0) {
            throw std::invalid_argument("invalid socket fd");
        }
        set_nonblocking();
    }
    explicit Socket(const Address &addr) : addr_(addr) {
        fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        if (fd_ < 0) {
            throw std::runtime_error(std::string("socket failed: ") + std::strerror(errno));
        }
        set_nonblocking();
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

    int fd() const {
        return fd_;
    }

    auto connect() {
        set_blocking();
        auto res = ::connect(fd_, addr_.socket_address(), addr_.size());
        set_nonblocking();
        return res;
    }
    auto bind() {
        return ::bind(fd_, addr_.socket_address(), addr_.size());
    }
    auto listen(int backlog = 128) const {
        return ::listen(fd_, backlog);
    }
    auto accept() const {
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

    void close() noexcept {
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
    }

    ~Socket() {
        close();
    }
};

export class Epoll {
    int fd_;
public:
    Epoll() {
        fd_ = epoll_create1(0);
        if (fd_ < 0)
            throw std::runtime_error("epoll_create1 failed");
    }

    void add(int fd, uint32_t events, void *ptr) {
        epoll_event ev{};
        ev.events = events;
        ev.data.ptr = ptr;
        if (epoll_ctl(fd_, EPOLL_CTL_ADD, fd, &ev) < 0)
            throw std::runtime_error("epoll add failed");
    }
    void add(int fd, uint32_t events) {
        epoll_event ev{};
        ev.events = events;
        ev.data.fd = fd;
        if (epoll_ctl(fd_, EPOLL_CTL_ADD, fd, &ev) < 0)
            throw std::runtime_error("epoll mod failed");

    }

    void mod(int fd, uint32_t events, void* ptr) {
        epoll_event ev{};
        ev.events = events;
        ev.data.ptr = ptr;
        if (epoll_ctl(fd_, EPOLL_CTL_MOD, fd, &ev) < 0)
            throw std::runtime_error("epoll mod failed");
    }

    void del(int fd) {
        epoll_ctl(fd_, EPOLL_CTL_DEL, fd, nullptr);
    }

    int wait(epoll_event *events, int max, int timeout = -1) {
        return epoll_wait(fd_, events, max, timeout);
    }

};

export class TCP {
    Socket socket_;
    std::vector<char> read_buf_;
    std::vector<char> write_buf_;
    bool is_listener_ = false;

public:
    TCP() = default;

    explicit TCP(const Address& addr)
        : socket_(addr) {}

    // 用于 accept() 返回的 Socket
    explicit TCP(Socket&& s)
        : socket_(std::move(s)) {}

    TCP(TCP&& other) noexcept
        : socket_(std::move(other.socket_)),
          read_buf_(std::move(other.read_buf_)),
          write_buf_(std::move(other.write_buf_)),
          is_listener_(other.is_listener_)
    {
        other.is_listener_ = false;
    }


    TCP& operator=(TCP&& other) noexcept {
        if (this != &other) {
            socket_ = std::move(other.socket_);
            read_buf_ = std::move(other.read_buf_);
            write_buf_ = std::move(other.write_buf_);
            is_listener_ = other.is_listener_;
            other.is_listener_ = false;
        }
        return *this;
    }


    [[nodiscard]] int fd() const { return socket_.fd(); }

    int connect() { return socket_.connect(); }
    int bind()    { return socket_.bind(); }

    [[nodiscard]] bool is_listener() const {
        return is_listener_;
    }
    int listen(int backlog = 128) {
        is_listener_ = true;
        return socket_.listen(backlog);
    }

    TCP accept() const {
        Socket s = socket_.accept();
        return TCP(std::move(s));
    }

    // -------------------------
    // 可读事件
    // -------------------------
    void get_message_impl() {
        if (is_listener_)
            return;

        char buf[4096];

        while (true) {
            int n = socket_.recv(std::span<char>(buf, sizeof(buf)));

            if (n > 0) {
                read_buf_.insert(read_buf_.end(), buf, buf + n);
            }
            else if (n == -1 && errno == EAGAIN) {
                break; // 读完了
            }
            else {
                close();
                break;
            }
        }
    }

    // -------------------------
    // 可写事件
    // -------------------------
    void send_message_impl() {
        while (!write_buf_.empty()) {
            int n = socket_.send(std::span<char>(write_buf_.data(), write_buf_.size()));

            if (n > 0) {
                write_buf_.erase(write_buf_.begin(), write_buf_.begin() + n);
            }
            else if (n == -1 && errno == EAGAIN) {
                break; // 下次再写
            }
            else {
                close();
                break;
            }
        }
    }

    // -------------------------
    // 外部状态机拉取完整包
    // -------------------------
    std::optional<std::vector<char>> get_message() {
        using length_type = int;

        if (read_buf_.size() < sizeof(length_type))
            return std::nullopt;

        length_type len{};
        std::memcpy(&len, read_buf_.data(), sizeof(len));

        if (read_buf_.size() < sizeof(len) + len)
            return std::nullopt;

        std::vector<char> msg(len);
        std::memcpy(msg.data(), read_buf_.data() + sizeof(len), len);

        read_buf_.erase(read_buf_.begin(),
                        read_buf_.begin() + sizeof(len) + len);

        return msg;
    }

    // -------------------------
    // 发送消息（自动进入 write_buf_）
    // -------------------------
    void send_message(std::span<const char> msg) {
        using length_type = int;

        length_type len = msg.size();

        write_buf_.insert(write_buf_.end(),
                          reinterpret_cast<const char*>(&len),
                          reinterpret_cast<const char*>(&len) + sizeof(len));

        write_buf_.insert(write_buf_.end(), msg.begin(), msg.end());
    }

    void send_now(std::span<char> msg) {
        send_message(msg);
        send_message_impl();
    }
    auto recv_now() {
        get_message_impl();
        return get_message();
    }

    // -------------------------
    // 关闭连接
    // -------------------------
    void close() {
        socket_.close();   // 你需要给 Socket 添加一个 public close()
        read_buf_.clear();
        write_buf_.clear();
    }

    ~TCP() {
        close();
    }
};

export {
    constexpr int epoll_in = EPOLLIN;
    constexpr int epoll_out = EPOLLOUT;
    constexpr int epoll_et = EPOLLET;
    constexpr int epoll_hup = EPOLLHUP;
    constexpr int epoll_err = EPOLLERR;
}

export {
    using ::epoll_event;
}


export using ::ssize_t;
