/********************************************************************************
* @Author : hexne
* @Date   : 2026/05/01 17:48:37
********************************************************************************/

module;
export module server;
import log;
import net;
import std;

class Server {
    std::vector<Socket> clients_;
    Socket listen_socket_;
public:
    Server() {
        listen_socket_ = Socket(Address{"0.0.0.0", 10086});
    }
    void accept() {
        auto new_client = listen_socket_.accept();
        if (new_client)
            clients_.push_back(new_client.value());
    }

};

export void server_main() {
    Log().push_log("Server start");
    Server server;

    while (true) {
        // 获取新连接

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }


}

