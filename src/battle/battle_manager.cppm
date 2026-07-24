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

export class BattleManager {
    std::unordered_map<int, std::shared_ptr<Battle>> battles_;
    BattleIDGenerator battle_id_generator_;
public:
    BattleManager() = default;

    void add_battle(const std::vector<int> &team_a, std::vector<int> team_b) {
        int id = battle_id_generator_.next();
        if (battles_.contains(id))
            return;
        battles_[id] = std::make_shared<Battle>(Team{ team_a }, Team{ team_b });
    }

    std::shared_ptr<Battle> get_battle(int id) {
        if (!battles_.contains(id))
            return {};
        return battles_[id];
    }

    void user_pick_hero(int battle_id, int user_id, HeroName hero_name) {
        auto battle = get_battle(battle_id);
        if (!battle)
            return;

        battle->pick_hero(user_id, hero_name);
    }


};
