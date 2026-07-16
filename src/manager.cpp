#include <cerrno>
import std;
import client;
import net;
import range;


class ClientManager {
    std::map<int, std::unique_ptr<Client>> users_;
    Epoll epoll_;
    bool stop_{};

    Client* research_user_by_id(const int id) {
        for (auto &[index, client] : users_) {
            if (client->user_id() == id) {
                return client.get();
            }
        }
        return nullptr;
    }
    int get_id_by_index(int index) {
        return users_[index]->user_id();
    }
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
        std::println("{:^10}{:^10}{:^10}{:^20}{:^20}{:^10}{:^10}", "index", "fd", "id", "name", "number", "status", "room");
        for (auto &[index, client] : users_) {
            std::println("{:^10}{:^10}{:^10}{:^20}{:^20}{:^10}{:^10}",
                index,
                client->fd(),
                client->user_id(),
                client->user_name(),
                client->user_number(),
                client->user_status(),
                client->room_id()
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
            std::println(std::cout, "add client fd={}", fd);
        }
    }

    void login(std::vector<int> &indexs) {
        if (indexs.empty()) {
            for (auto &[index, client] : users_)
                indexs.push_back(index);
        }
        for (auto index : indexs) {
            auto &client = users_[index];
            // client->login();
            auto back = std::to_string(index + 1);
            client->login(std::format("num{}", back),
                std::format("pass{}", back));
        }
    }


    void room_create(std::vector<int> &indexs) {
        if (indexs.empty())
            return;

        for (auto index : indexs) {
            auto &client = users_[index];
            client->room_create();
        }
    }

    // index1 邀请 index2
    void room_invite(int user1_index, int user2_index) {
        auto &client = users_[user1_index];
        int id = get_id_by_index(user2_index);
        client->room_invite(id);
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
    static void end() {
        std::print(">>");
    }
};




int main(int argc, char *argv[]) {
    ClientManager manager;
    std::jthread thread(&ClientManager::epoll_loop, &manager);

    std::ifstream file{};
    std::istream *in{};
    if (argc != 1) {
        auto path = std::filesystem::path(argv[1]);
        if (!std::filesystem::exists(path)) {
            std::println("file not exist {}", path);
        }
        file = std::ifstream(path);
        in = &file;
    }
    else {
        in = &std::cin;
    }




    Command command;
    command.end();
    while (*in >> command) {
        if (command.cmd == "quit" || command.cmd == "q" || command.cmd == "exit") {
            manager.stop();
            thread.request_stop();
            break;
        }
        if (command.cmd == "h" || command.cmd == "help") {  }

        else if (command.cmd  == "show" || command.cmd == "ls") {
            manager.show();
        }
        // add number, 添加number个客户端
        else if (command.cmd == "add") {
            int number = std::stoi(command.args.front());
            manager.add(number);
        }
        else if (command.cmd == "login") {
            std::vector<int> indexs{};
            // login
            if (command.args.empty()) {
                manager.login(indexs);
                command.end();
                continue;
            }
            auto arg = command.args.front();
            // login 0-2
            if (arg.find('-')) {
                auto pos = arg.find('-');

                int begin = std::stoi(arg.substr(0, pos));
                int end   = std::stoi(arg.substr(pos + 1));

                for (auto i : Range(begin, end + 1)) {
                    indexs.push_back(i);
                }
            }
            // login 0
            // login 0 2 5
            else if (!command.args.empty()) {
                for (auto arg : command.args) {
                    indexs.push_back(std::stoi(arg));
                }
            }
            manager.login(indexs);
        }
        // room 所有用户各自进入一个房间
        // room 0 用户0进入房间
        // room 0 1 2 3 4 0-4 进入同一个房间
        else if (command.cmd == "room") {
            // 所有用户各自进入一个房间
            if (command.args.empty()) {

            }
            else {
                std::vector<int> indexs{};
                for (auto arg : command.args) {
                    indexs.emplace_back(std::stoi(arg));
                }
                manager.room_create(indexs);
            }

        }
        else if (command.cmd == "room_invite") {

            int user = std::stoi(command.args.front());
            for (int i = 1; i < command.args.size(); ++i) {
                auto user2 = std::stoi(command.args[i]);
                manager.room_invite(user, user2);
            }
        }
        command.end();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    thread.join();

    return 0;
}