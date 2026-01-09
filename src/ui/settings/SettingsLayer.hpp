#pragma once

#include <globed/prelude.hpp>
#include <ui/BaseLayer.hpp>
#include "cells/BaseSettingCell.hpp"

#include <cue/ListNode.hpp>

namespace globed {

class SettingsLayer : public BaseLayer {
public:
    static SettingsLayer* create();
    static constexpr cocos2d::CCSize CELL_SIZE = {356.f,28.f};

private:
    Ref<cue::ListNode> m_globedTab;
    Ref<cue::ListNode> m_playersTab;
    Ref<cue::ListNode> m_levelUiTab;
    Ref<cue::ListNode> m_voiceTab;
    Ref<cue::ListNode> m_menusTab;
    std::vector<cue::ListNode*> m_tabs;
    cue::ListNode* _m_curTab = nullptr;
    cue::ListNode* m_selectedTab = nullptr;
    std::vector<geode::TabButton*> m_tabButtons;

    cocos2d::CCClippingNode* m_tabsClipper = nullptr;
    CCSprite* m_tabsGradientSpr = nullptr;
    CCSprite* m_tabsGradientStencil = nullptr;

    bool init();
    void addSettings();

    void selectTab(size_t idx);
    void selectTab(cue::ListNode* tab);

    template <typename T, typename... Args>
    T* addSetting(CStr key, CStr name, CStr desc, Args&&... args) {
        auto cell = T::create(key, name, desc, std::forward<Args>(args)..., CELL_SIZE);
        this->addSetting(cell);
        return cell;
    }

    void addHeader(CStr key, CStr text, cue::ListNode* curTab);
    void addSetting(BaseSettingCellBase* cell);

    void refreshAll();
    void onTabClick(CCObject* sender);
};

}