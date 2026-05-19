import std;
import client;
import net;

// std::vector<std::unique_ptr<Client>> clients;
//
struct Account {
    std::string name;
    std::string number;
    std::string password;
};

Account get_account(int number) {
    return Account {
        .name = std::format("user{}", number),
        .number = std::format("num{}", number),
        .password = std::format("pass{}", number),
    };
}


void create_client(std::span<char> arg) {
    int number = std::stoi(std::string(arg.data(), arg.size()));
    clients.reserve(number);
    for (int i = 0; i < number; i++) {
        clients.emplace_back(std::make_unique<Client>(Address{"127.0.0.1", 8080}));
    }
    std::cout << "client : " << clients.size() << std::endl;
}
//
// void login(std::span<char> arg) {
//     int number = std::stoi(std::string(arg.data(), arg.size()));
//     auto count = std::min(static_cast<std::size_t>(number), clients.size());
//     std::size_t success = 0;
//
//     for (std::size_t i = 0; i < count; i++) {
//         auto account = get_account(static_cast<int>(i + 1));
//         if (clients[i]->login(account.number, account.password))
//             success++;
//     }
//
//     std::cout << "login : " << success << "/" << count << std::endl;
// }
//
// void reg(std::span<char> arg) {
//     int number = std::stoi(std::string(arg.data(), arg.size()));
//
//     for (std::size_t i = 0; i < number; i++) {
//         auto account = get_account(static_cast<int>(i + 1));
//         clients[i]->register_user(account.name, account.number, account.password);
//     }
// }
//
struct UserStatus {
    int id{};
    std::string name{};
    std::string status{};
    std::unique_ptr<Client> client;
};

std::vector<UserStatus> users;



void show_user_status() {
    users[0].client->
    std::println("{:^20}{:^20}{:^20}", "id", "name", "status");

    for (auto user : users) {
        std::println("{:^20}{:^20}{:^20}", user.id, user.name, user.status);
    }
}

int main(int argc, char *argv[]) {
    // std::thread thread(show_user_status);

    std::string cmd;

    std::print(":");
    while (std::cin >> cmd) {
        if (cmd == "quit" || cmd == "q" || cmd == "exit") {
            break;
        }
        if (cmd == "h" || cmd == "help") {  }

        else if (cmd == "show") {
            show_user_status();
        }
        else if (cmd == "add") {
            int number;
            std::cin >> number;

            // 创建多少个用户
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

    return 0;
}
