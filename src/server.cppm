/********************************************************************************
* @Author : hexne
* @Date   : 2026/05/01 17:48:37
********************************************************************************/

module;
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
export module server;
import log;
import net;
import database;
import std;

export class Server {
public:

};


export void server_main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 5);

    printf("Server listening...\n");

    int client_fd = accept(server_fd, NULL, NULL);

    char buf[1024];
    while (1) {
        ssize_t n = recv(client_fd, buf, sizeof(buf), 0);
        if (n <= 0) break;
        send(client_fd, buf, n, 0);
    }

    close(client_fd);
    close(server_fd);
}
