/********************************************************************************
* @Author : hexne
* @Date   : 2026/05/01 17:49:03
********************************************************************************/

module;
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
export module client;
import log;
import std;
import std.compat;
import net;
import user;

export class Client {
public:

};

export void client_main() {
    sleep(1); // 等 server 启动

    int fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    connect(fd, (struct sockaddr*)&addr, sizeof(addr));

    const char* msg = "Hello Echo Server!";
    send(fd, msg, strlen(msg), 0);

    char buf[1024];
    ssize_t n = recv(fd, buf, sizeof(buf), 0);
    if (n > 0) {
        printf("Client received: %.*s\n", (int)n, buf);
    }

    close(fd);
}