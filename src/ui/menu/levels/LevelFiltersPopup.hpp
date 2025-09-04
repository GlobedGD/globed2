#pragma once

#include <globed/util/gd.hpp>
#include <ui/BasePopup.hpp>
#include "LevelListLayer.hpp"

#include <Geode/Geode.hpp>

namespace globed {

class LevelFiltersPopup : public BasePopup<LevelFiltersPopup, LevelListLayer*> {
public:
    using Filters = LevelListLayer::Filters;
    using Callback = std::function<void(Filters)>;

    static const cocos2d::CCSize POPUP_SIZE;

    void setCallback(Callback&& cb);

protected:
    geode::Ref<LevelListLayer> m_layer;
    Callback m_callback;
    Filters m_filters;
    CCMenuItemToggler* m_btnCompleted;
    CCMenuItemToggler* m_btnUncompleted;
    std::map<globed::Difficulty, CCMenuItemSpriteExtra*> diffButtons;

    bool setup(LevelListLayer* layer) override;
    void onClose(cocos2d::CCObject*) override;
};

}

