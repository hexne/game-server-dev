/********************************************************************************
* @Author : hexne
* @Date   : 2026/08/01 15:07:27
********************************************************************************/

module;
#include <nlohmann/json.hpp>
export module config;

export struct Config {
    std::string server_listen_ip{};
    int server_listen_port{};

    std::string database_host{};
    int database_port{};
    std::string database_name{};
    std::string database_user{};
    std::string database_password{};

    Config() {
        static constexpr unsigned char config_str[] = {
#embed "src/server/server_config.json"
        };

        std::string json_str(reinterpret_cast<const char*>(config_str), sizeof(config_str));

        // 解析 JSON
        nlohmann::json config = nlohmann::json::parse(json_str);

        // 读取 server_config
        server_listen_ip   = config["server_config"]["server_listen_ip"].get<std::string>();
        server_listen_port = config["server_config"]["server_listen_port"].get<int>();

        // 读取 database_config
        database_host      = config["database_config"]["host"].get<std::string>();
        database_port      = config["database_config"]["port"].get<int>();
        database_name      = config["database_config"]["name"].get<std::string>();
        database_user      = config["database_config"]["user"].get<std::string>();
        database_password  = config["database_config"]["password"].get<std::string>();
    }
};



export Config& config() {
    static Config config;
    return config;
};