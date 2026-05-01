import std;
import server;
import client;

int main() {
    auto server = std::thread(Server);
    auto client = std::thread(Client);
    server.join();
    client.join();
}
