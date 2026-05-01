/********************************************************************************
* @Author : hexne
* @Date   : 2026/05/01 18:08:30
********************************************************************************/

module;
export module user;
import std;
import net;

export struct User {
    int id{};
    std::string name{};
    TCP tcp_{};
    UDP udp_{};
};