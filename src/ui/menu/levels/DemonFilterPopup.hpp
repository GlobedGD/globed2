#pragma once

#include <globed/util/gd.hpp>
#include <ui/BasePopup.hpp>
#include "LevelListLayer.hpp"

#include <Geode/Geode.hpp>
#include <std23/move_only_function.h>

namespace globed {

class DemonFilterPopup : public BasePopup<DemonFilterPopup, const LevelListLayer::Filters&> {
public:
    // Callback takes in a bool and set<util::gd::Difficulty>&&, bool is true if user chose to filter by demons, false otherwise
    using Callback = std23::move_only_function<void (bool, std::set<Difficulty>&&)>;

    static const cocos2d::CCSize POPUP_SIZE;

    void setCallback(Callback&& cb);
private:
    std::set<Difficulty> m_demonTypes;
    std::map<Difficulty, CCMenuItemSpriteExtra*> m_demonButtons;
    CCMenuItemSpriteExtra* m_generalDemonButton;
    Callback m_callback;
    bool m_isFilteringGeneral = false;

    bool setup(const LevelListLayer::Filters&) override;

    void onClose(cocos2d::CCObject*) override;
};

}
