#include <cerrno>
#include <cstring>
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
            if (n == -1) {
                if (errno == EINTR)
                    continue;
            }
            for (int i : Range(n)) {
                auto *client = static_cast<Client*>(events[i].data.ptr);
                auto ev = events[i].events;
                if (ev & epoll_out) {
                    client->tcp().send_message_impl();
                }
                if (ev & epoll_in) {
                    client->tcp().get_message_impl();
                }

                while (auto msg = client->tcp().get_message()) {
                    client->rounter(msg.value());
                }
            }

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

    void login(const std::vector<int> &indexs) {
        for (auto index : indexs) {
            auto &client = users_[index];
            // client->login();
            auto back = std::to_string(index + 1);
            client->login(std::format("num{}", back),
                std::format("pass{}", back));
        }
    }


    void stop() {
        stop_ = true;
    }

};



struct Command {
    std::string cmd;
    std::vector<std::string> args;

    friend std::istream& operator>>(std::istream& is, Command& out) {
        out.cmd.clear();
        out.args.clear();

        std::string line;
        if (!std::getline(is, line))
            return is;

        if (line.empty())
            return is;

        std::istringstream iss(line);

        // 读取命令
        iss >> out.cmd;

        // 读取所有参数
        std::string arg;
        while (iss >> arg)
            out.args.push_back(arg);

        return is;
    }
};


int main(int argc, char *argv[]) {
    ClientManager manager;
    std::jthread thread(&ClientManager::epoll_loop, &manager);

    Command command;

    std::print(":");
    while (std::cin >> command) {
        if (command.cmd == "quit" || command.cmd == "q" || command.cmd == "exit") {
            manager.stop();
            thread.request_stop();
            break;
        }
        if (command.cmd == "h" || command.cmd == "help") {  }

        else if (command.cmd  == "show") {
            manager.show();
        }
        // add number, 添加number个客户端
        // @TODO，add 的时候就应该创建fd,而不是在login的时候
        else if (command.cmd == "add") {
            int number = std::stoi(command.args.front());
            manager.add(number);
        }
        else if (command.cmd == "login") {
            std::vector<int> indexs{};
            auto arg = command.args.front();
            // up 0-2
            if (arg.find('-')) {
                auto pos = arg.find('-');

                int begin = std::stoi(arg.substr(0, pos));
                int end   = std::stoi(arg.substr(pos + 1));

                for (auto i : Range(begin, end + 1)) {
                    indexs.push_back(i);
                }
            }
            // up 0, 2, 5
            else if (arg.find(',')) {

            }
            // up id
            else {
                arg.push_back(std::stoi(arg));
            }
            manager.login(indexs);
        }

        std::print(":");
    }
    thread.join();

    return 0;
}