#pragma once
#include <defs/all.hpp>

#include "setting_cell.hpp"

class GlobedSettingsLayer : public cocos2d::CCLayer {
public:
    static constexpr float LIST_WIDTH = 358.f;
    static constexpr float LIST_HEIGHT = 220.f;
    static constexpr int TAG_TAB_GLOBED = 1;
    static constexpr int TAG_TAB_MENUS = 2;
    static constexpr int TAG_TAB_COMMUNICATION = 3;
    static constexpr int TAG_TAB_LEVELUI = 4;
    static constexpr int TAG_TAB_PLAYERS = 5;

    static GlobedSettingsLayer* create();

protected:
    std::unordered_map<int, Ref<GJListLayer>> storedTabs;
    geode::TabButton *tabBtn1, *tabBtn2, *tabBtn3, *tabBtn4, *tabBtn5;
#ifdef GEODE_IS_IOS // TODO
    cocos2d::CCNode* tabsGradientNode = nullptr;
#else
    cocos2d::CCClippingNode* tabsGradientNode = nullptr;
#endif

    cocos2d::CCSprite *tabsGradientSprite, *tabsGradientStencil;
    std::unordered_map<int, Ref<cocos2d::CCArray>> settingCells;
    int currentTab = -1;

    bool init() override;
    void keyBackClicked() override;
    void onTab(cocos2d::CCObject* sender);

    void remakeList();
    void createSettingsCells(int category);
    void addHeader(int category, const char* name);
    GJListLayer* makeListLayer(int category);

public:
    template <typename T>
    constexpr static GlobedSettingCell::Type getCellType() {
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