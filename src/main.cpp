#include <arpa/inet.h>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>
//
//
// void set_nonblocking(int fd) {
//     int flags = fcntl(fd, F_GETFL, 0);
//     fcntl(fd, F_SETFL, flags | O_NONBLOCK);
// }
//
// void Server() {
//     Log().push_log("启动Server线程");
//     int epoll_fd = epoll_create1(0);
//
//     // 创建socket, 但是还没有地址
//     int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
//
//     epoll_event ev{};
//     ev.events = EPOLLIN;
//     ev.data.fd = listen_fd;
//
//     epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev);
//
//
//     // 设置地址
//     sockaddr_in addr{};
//     addr.sin_family = AF_INET;
//     // 端口转为网络字节序
//     addr.sin_port = htons(8080);
//     // 监听所有ip, 等于0.0.0.0
//     addr.sin_addr.s_addr = INADDR_ANY;
//
//     // socket 和 地址绑定在一块
//     bind(listen_fd, (sockaddr*)&addr, sizeof(addr));
//
//     // 开始监听， somaxconn 可以排队多少 等待 accept的连接
//     listen(listen_fd, SOMAXCONN);
//
//     // 设置非阻塞
//     set_nonblocking(listen_fd);
//
//     epoll_event events[1024];
//     while (true) {
//         int n = epoll_wait(epoll_fd, events, 1024, -1);
//
//         for (int i = 0; i < n; i++) {
//             int fd = events[i].data.fd;
//
//             if (fd == listen_fd) {
//                 while (true) {
//                     int client_fd = accept(listen_fd, nullptr, nullptr);
//
//                     if (client_fd < 0)
//                         break;
//                     set_nonblocking(client_fd);
//
//                     epoll_event ev{};
//                     ev.events = EPOLLIN;
//                     ev.data.fd = client_fd;
//                     epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
//                 }
//             }
//             else {
//                 char buffer[1024];
//                 while (true) {
//                     int n = read(fd, buffer, sizeof(buffer));
//                     if (n > 0) {
//                         write(fd, buffer, n);
//                     }
//                     else if (n == 0) {
//                         close(fd);
//                         break;
//                     }
//                     else {
//                         if (errno == EAGAIN)
//                             break;
//                         close(fd);
//                         break;
//                     }
//                 }
//             }
//         }
//
//         std::this_thread::sleep_for(std::chrono::milliseconds(100));
//     }
// }
//
// void Client() {
//     std::this_thread::sleep_for(std::chrono::seconds{1});
//     Log().push_log("启动Client线程");
//     int sock = socket(AF_INET, SOCK_STREAM, 0);
//
//     sockaddr_in server_addr{};
//     server_addr.sin_family = AF_INET;
//     server_addr.sin_port = htons(8080);
//     inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
//
//     connect(sock, (sockaddr*)&server_addr, sizeof(server_addr));
//
//     char buffer[1024];
//     int counter{};
//
//     while (true) {
//         std::string msg = "hello " + std::to_string(counter++);
//         write(sock, msg.c_str(), msg.size());
//
//         Log().push_log("发送: " + msg);
//
//         int n = read(sock, buffer, sizeof(buffer));
//         if (n > 0) {
//             std::string echo(buffer, n);
//             Log().push_log("收到回声: " + echo);
//         }
//
//         std::this_thread::sleep_for(std::chrono::milliseconds(100));
//     }
// }
//

//
// void server() {
//     // 指定监听地址
//     // 将收到的信息返回
// }
//
// void client() {
//     constexpr int thread_count = 6;
//
//     std::array<std::vector<int>, thread_count> fps;
//     std::array<std::thread, thread_count> threads;
//     for (int i = 0; i < thread_count; i++) {
//
//         threads[i] = std::thread([&](int index) {
//             auto &vec = fps[index];
//             // 随机获得0, 1
//             // 如果有连接
//             //     如果是0, 尝试连接
//             //     如果是1, 发送消息
//             // 如果没有连接
//             //     创建连接
//             //
//
//
//
//         });
//
//     }
//     // 多个线程同时
//     // 随机[0, 1]
//     // 0 发送消息
//     // 1 创建新连接, 放到vector中
//
// }
// #include <sys/socket.h>
import std;
import log;
import net;

void Server() {
    Log().push_log("Server start");

    // 服务端绑定到 8080
    Socket server(Address{"0.0.0.0", 8080});
    server.listen();

    // 接受一个客户端连接
    Socket client;
    while (true) {
        auto client_socket = server.accept();
        if (client_socket) {
            client = std::move(client_socket.value());
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    Log().push_log("Server accepted client");

    // 用一个 Socket 包装这个 fd
    // 注意：这里可以扩展 Socket 构造函数支持接收已有 fd

    char buf[1024];
    while (true) {
        ssize_t n = client.read(buf, sizeof(buf));
        if (n <= 0) {
            Log().push_log("Server client closed");
            break;
        }
        buf[n] = '\0';
        Log().push_log(std::string("Server received: ") + buf);

        // 回显给客户端
        client.write(buf, n);
    }
}

void Client() {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    Log().push_log("Client start");

    Socket client; // 自动分配本地地址
    Address server_addr("127.0.0.1", 8080);
    client.connect(server_addr);

    const char msg[] = "Hello Echo Server!";
    client.write(msg);

    char buf[1024];
    ssize_t n = client.read(buf, sizeof(buf));
    if (n > 0) {
        buf[n] = '\0';
        Log().push_log(std::string("Client received: ") + buf);
    }
}


int main() {
    auto server = std::thread(Server);
    auto client = std::thread(Client);
    server.join();
    client.join();
}
