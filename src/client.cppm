/********************************************************************************
* @Author : hexne
* @Date   : 2026/05/01 17:49:03
********************************************************************************/

module;
export module client;
import log;
import std;

class Client {

public:

};

export void client_main() {
    Log().push_log("Client start");
    Client client;

    while (true) {


        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }


}