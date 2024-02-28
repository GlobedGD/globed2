#pragma once
#include <defs/all.hpp>

#include "setting_cell.hpp"
#include "setting_header_cell.hpp"

class GlobedSettingsLayer : public cocos2d::CCLayer {
public:
    static constexpr float LIST_WIDTH = 358.f;
    static constexpr float LIST_HEIGHT = 220.f;
    static constexpr int TAG_TAB_GLOBED = 1;
    static constexpr int TAG_TAB_OVERLAY = 2;
    static constexpr int TAG_TAB_COMMUNICATION = 3;
    static constexpr int TAG_TAB_LEVELUI = 4;
    static constexpr int TAG_TAB_PLAYERS = 5;

    static GlobedSettingsLayer* create();

protected:
    std::unordered_map<int, Ref<GJListLayer>> storedTabs;
    geode::TabButton *tabBtn1, *tabBtn2, *tabBtn3, *tabBtn4, *tabBtn5;
    cocos2d::CCClippingNode* tabsGradientNode = nullptr;
    cocos2d::CCSprite *tabsGradientSprite, *tabsGradientStencil;
    int currentTab = -1;

    bool init() override;
    void keyBackClicked() override;
    void onTab(cocos2d::CCObject* sender);

    void remakeList();
    cocos2d::CCArray* createSettingsCells(int category);
    GJListLayer* makeListLayer(int category);

    template <typename T>
    constexpr GlobedSettingCell::Type getCellType() {
        using Type = GlobedSettingCell::Type;

        if constexpr (std::is_same_v<T, bool>) {
            return Type::Bool;
        } else if constexpr (std::is_same_v<T, float>) {
            return Type::Float;
        } else if constexpr (std::is_same_v<T, int>) {
            return Type::Int;
        } else {
            static_assert(std::is_same_v<T, void>, "invalid type for GlobedSettingLayer::getCellType");
        }
    }
};