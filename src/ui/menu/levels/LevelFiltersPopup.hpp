#pragma once

#include <globed/util/gd.hpp>
#include <ui/BasePopup.hpp>
#include "LevelListLayer.hpp"

#include <Geode/Geode.hpp>
#include <Geode/utils/function.hpp>

namespace globed {

class LevelFiltersPopup : public BasePopup {
public:
    using Filters = LevelListLayer::Filters;
    using Callback = geode::Function<void(Filters)>;

    static LevelFiltersPopup* create(LevelListLayer* layer);

    void setCallback(Callback&& cb);

protected:
    geode::Ref<LevelListLayer> m_layer;
    Callback m_callback;
    Filters m_filters;
    CCMenuItemToggler* m_btnCompleted;
    CCMenuItemToggler* m_btnUncompleted;
    std::map<globed::Difficulty, CCMenuItemSpriteExtra*> diffButtons;

    bool init(LevelListLayer* layer);
    void onClose(cocos2d::CCObject*) override;
};

}

