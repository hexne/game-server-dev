/********************************************************************************
* @Author : hexne
* @Date   : 2026/07/24 14:09:41
********************************************************************************/

module;
export module hero_factory;
import std;
import hero;

export namespace HeroFactory {
    std::shared_ptr<Hero> create_hero(HeroName hero_name) {
        switch (hero_name) {
        case HeroName::bjorn:
            return std::make_shared<Bjorn>();
        case HeroName::loki:
            return std::make_shared<Loki>();
        case HeroName::merlin:
            return std::make_shared<Merlin>();
        case HeroName::thorn:
            return std::make_shared<Thorn>();
        }
        return nullptr;
    }

}