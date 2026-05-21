import std;
import client;
import net;
import range;


class ClientManager {
    std::map<int, std::unique_ptr<Client>> users_;
    Epoll epoll_;
    bool stop_{};
public:
    void epoll_loop() {

        while (!stop_) {
            epoll_event events[128];
            int n = epoll_.wait(events, 128, 100);
        }

    }

    void show() {
        std::println("{:^10}{:^10}{:^20}{:^20}{:^20}{:^20}", "index", "fd", "id", "name", "number", "status");
        for (auto &[index, client] : users_) {
            std::println("{:^10}{:^10}{:^20}{:^20}{:^20}",
                index,
                client->fd(),
                client->user_id(),
                client->user_name(),
                client->user_number()
                );

        }
        // for (auto &[fd, user] : users_) {
            // std::println("{:^10}{:^10}{:^20}{:^20}{:^20}", fd, "/", user.name, "/");
        // }
    }

    void add(int number = 1) {
        static int index{};
        for (int i : Range(number)) {
            auto client = std::make_unique<Client>(Address{"127.0.0.1", 8080});
            int fd = client->fd();
            epoll_.add(fd, epoll_in | epoll_out | epoll_et, client.get());
            users_.emplace(index ++, std::move(client));
            std::println("add client fd={}", fd);
        }
    }


    void stop() {
        stop_ = true;
    }

};



void epoll_thread() {

}


int main(int argc, char *argv[]) {
    ClientManager manager;
    std::jthread thread(&ClientManager::epoll_loop, &manager);

    std::string cmd;

    std::print(":");
    while (std::cin >> cmd) {
        if (cmd == "quit" || cmd == "q" || cmd == "exit") {
            manager.stop();
            thread.request_stop();
            break;
        }
        if (cmd == "h" || cmd == "help") {  }

        else if (cmd == "show") {
            manager.show();
        }
        // add number, 添加number个客户端
        // @TODO，add 的时候就应该创建fd,而不是在login的时候
        else if (cmd == "add") {
            int number;
            std::cin >> number;
            manager.add(number);
        }
        else if (cmd == "up") {
            std::vector<int> ids{};
            std::string id;
            // up 0-2
            if (id.find('-')) {

            }
            // up 0, 2, 5
            else if (id.find(',')) {

            }
            // up id
            else {
                ids.push_back(std::stoi(id));
            }
            // 取id, 分别login
        }

        std::print(":");
    }
    thread.join();

    return 0;
}
