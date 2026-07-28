module;
#include <cerrno>
export module client_manager;
import std;
import client;
import net;
import range;


export class ClientManager {
    std::map<int, std::unique_ptr<Client>> users_;
    Epoll epoll_;
    bool stop_{};
    std::thread thread_;

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

    ClientManager() {
        thread_ = std::thread(&ClientManager::epoll_loop, this);
    }

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

    void match(int index) {
        auto &client = users_[index];
        client->match_join();
    }

    void stop() {
        stop_ = true;
    }

    ~ClientManager() {
        stop();
        thread_.join();
    }
};
