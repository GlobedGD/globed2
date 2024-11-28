#pragma once

#include <Geode/ui/Popup.hpp>
#include "level_list_layer.hpp"

class GlobedDemonFilterPopup : public geode::Popup<
    const GlobedLevelListLayer::Filters&,
    std::function<void (bool, std::set<util::gd::Difficulty>&&)>&&
> {
public:
    using Callback = std::function<void (bool, std::set<util::gd::Difficulty>&&)>;

    // Callback takes in a bool and set<util::gd::Difficulty>&&, bool is true if user chose to filter by demons, false otherwise
    static GlobedDemonFilterPopup* create(const GlobedLevelListLayer::Filters&, Callback&& cb);

private:
    std::set<util::gd::Difficulty> demonTypes;
    std::map<util::gd::Difficulty, CCMenuItemSpriteExtra*> demonButtons;
    CCMenuItemSpriteExtra* generalDemonButton;
    Callback callback;
    bool isFilteringGeneral = false;

    bool setup(const GlobedLevelListLayer::Filters&, Callback&& cb) override;

    void onClose(cocos2d::CCObject*) override;
};
