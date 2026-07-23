/********************************************************************************
* @Author : hexne
* @Date   : 2026/07/23 21:31:06
********************************************************************************/

module;
export module battle_manager;
import battle;
import id_generator;
import std;

export class BattleManager {
    std::vector<std::shared_ptr<Battle>> battles_;

public:
    BattleManager() = default;


};
