/********************************************************************************
* @Author : hexne
* @Date   : 2026/07/23 21:31:06
********************************************************************************/

module;
export module battle_manager;
import battle;
import id_generator;
import team;
import std;
import hero;

export class BattleManager {
    std::unordered_map<int, std::shared_ptr<Battle>> battles_{};
    BattleIDGenerator battle_id_generator_{};
    std::default_random_engine random_engine_{};
public:
    BattleManager() : random_engine_(std::random_device{}()) {  }

    int add_battle(const std::vector<int> &team_a, const std::vector<int> &team_b) {
        const int id = battle_id_generator_.next();
        battles_[id] = std::make_shared<Battle>(id, random_engine_(), Team{ team_a }, Team{ team_b }, BattleType::pick_hero);
        return id;
    }
    void battle_finish(int battle_id) {
        battles_.erase(battle_id);
    }

    std::shared_ptr<Battle> get_battle(int id) {
        if (!battles_.contains(id))
            return {};
        return battles_[id];
    }


    void battle_update_all_hero_pos() {
        for (auto& battle : battles_ | std::views::values) {
            battle->update_all_hero_pos();
        }
    }

    std::vector<int> all_battle_id() {
        std::vector<int> ret;
        for (auto id : battles_ | std::views::keys)
            ret.push_back(id);
        return ret;
    }
};
