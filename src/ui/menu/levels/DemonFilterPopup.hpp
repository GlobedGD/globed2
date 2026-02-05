#pragma once

#include <globed/util/gd.hpp>
#include <ui/BasePopup.hpp>
#include "LevelListLayer.hpp"

#include <Geode/Geode.hpp>
#include <Geode/utils/function.hpp>

namespace globed {

class DemonFilterPopup : public BasePopup {
public:
    // Callback takes in a bool and set<util::gd::Difficulty>&&, bool is true if user chose to filter by demons, false otherwise
    using Callback = geode::Function<void (bool, std::set<Difficulty>&&)>;

    static DemonFilterPopup* create(const LevelListLayer::Filters& filters);

    void setCallback(Callback&& cb);
private:
    std::set<Difficulty> m_demonTypes;
    std::map<Difficulty, CCMenuItemSpriteExtra*> m_demonButtons;
    CCMenuItemSpriteExtra* m_generalDemonButton;
    Callback m_callback;
    bool m_isFilteringGeneral = false;

    bool init(const LevelListLayer::Filters&);

    void onClose(cocos2d::CCObject*) override;
};

}
