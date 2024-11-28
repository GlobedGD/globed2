#pragma once

#include <Geode/ui/Popup.hpp>
#include "level_list_layer.hpp"

class GlobedLevelFilterPopup : public geode::Popup<
    const GlobedLevelListLayer::Filters&,
    GlobedLevelListLayer*,
    std::function<void (const GlobedLevelListLayer::Filters&)>&&
> {
public:
    using Callback = std::function<void (const GlobedLevelListLayer::Filters&)>;

    static GlobedLevelFilterPopup* create(const GlobedLevelListLayer::Filters&, GlobedLevelListLayer* ll, Callback&& cb);

private:
    geode::Ref<GlobedLevelListLayer> levelListLayer;
    GlobedLevelListLayer::Filters filters;
    Callback callback;
    CCMenuItemToggler* btnCompleted;
    CCMenuItemToggler* btnUncompleted;
    std::map<util::gd::Difficulty, CCMenuItemSpriteExtra*> diffButtons;

    bool setup(const GlobedLevelListLayer::Filters&, GlobedLevelListLayer* ll, Callback&& cb) override;

    void onClose(cocos2d::CCObject*) override;
};
