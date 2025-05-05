#pragma once
#include <defs/all.hpp>

class GlobedSettingsLayer : public cocos2d::CCLayer {
public:
    static constexpr float LIST_WIDTH = 358.f;
    static constexpr float LIST_HEIGHT = 220.f;
    static constexpr int TAG_TAB_GLOBED = 1;
    static constexpr int TAG_TAB_MENUS = 2;
    static constexpr int TAG_TAB_COMMUNICATION = 3;
    static constexpr int TAG_TAB_LEVELUI = 4;
    static constexpr int TAG_TAB_PLAYERS = 5;

    static GlobedSettingsLayer* create(bool showConnectionTest = false);

protected:
    std::unordered_map<int, Ref<GJListLayer>> storedTabs;
    geode::TabButton *tabBtn1, *tabBtn2, *tabBtn3, *tabBtn4, *tabBtn5;
    cocos2d::CCClippingNode* tabsGradientNode = nullptr;

    cocos2d::CCSprite *tabsGradientSprite, *tabsGradientStencil;
    std::unordered_map<int, Ref<cocos2d::CCArray>> settingCells;
    int currentTab = -1;
    size_t prevSettingsSlot = 0;
    bool doShowConnectionTest = false;

    bool init(bool showConnectionTest);
    void update(float dt) override;
    void keyBackClicked() override;
    void onEnterTransitionDidFinish() override;
    void onTab(cocos2d::CCObject* sender);
    void onTabById(int tag);

    void remakeList();
    void createSettingsCells(int category);
    void addHeader(int category, const char* name);
    GJListLayer* makeListLayer(int category);
};