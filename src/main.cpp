import std;
import server;
import client;

int main() {
    auto server = std::thread(server_main);
    // auto client = std::thread(client_main);
    server.join();
    // client.join();
}
