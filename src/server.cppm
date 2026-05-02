/********************************************************************************
* @Author : hexne
* @Date   : 2026/05/01 17:48:37
********************************************************************************/

module;
export module server;
import log;
import net;
import database;
import std;

class Server {
    std::vector<TCP> clients_;
    TCP listen_socket_;
public:
    Server() {
        listen_socket_ = TCP(Address{"0.0.0.0", 10086});
    }
    void accept() {
        auto new_client = listen_socket_.accept();
        if (new_client)
            clients_.emplace_back(std::move(new_client.value()));
    }

};

export void server_main() {
    Log().push_log("Server start");
    Server server;


    Database db("root", "arch", "game");
    for (auto row : db->select("id", "name")
                        .from("users")
                        .exec())
    {
        std::cout << row.get_int(0) << " " << row.get_string(1) << "\n";
    }


}

