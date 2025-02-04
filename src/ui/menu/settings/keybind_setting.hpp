#pragma once

#include <Geode/loader/SettingV3.hpp>
#include <Geode/loader/Mod.hpp>

#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>

class KeybindSetting : public geode::SettingV3 {
public:
    static geode::Result<std::shared_ptr<geode::SettingV3>> parse(std::string const& key, std::string const& modID, matjson::Value const& json);
    bool load(matjson::Value const& json) override {
        return true;
    }
    bool save(matjson::Value& json) const override {
        return true;
    }
    bool isDefaultValue() const override {
        return true;
    }
    void reset() override {}

    geode::SettingNodeV3* createNode(float width) override;
};

class KeybindSettingNode : public geode::SettingNodeV3 {
protected:
    ButtonSprite* m_buttonSprite;
    CCMenuItemSpriteExtra* m_button;

    bool init(std::shared_ptr<KeybindSetting> setting, float width);
    void onButton(cocos2d::CCObject*);
    void onCommit() override {}
    void onResetToDefault() override {}

public:
    static KeybindSettingNode* create(std::shared_ptr<KeybindSetting> setting, float width);
    bool hasUncommittedChanges() const override {
        return false;
    }
    bool hasNonDefaultValue() const override {
        return false;
    }
    std::shared_ptr<KeybindSetting> getSetting() const {
        return std::static_pointer_cast<KeybindSetting>(geode::SettingNodeV3::getSetting());
    }
};

