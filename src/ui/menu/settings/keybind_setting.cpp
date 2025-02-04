#include "keybind_setting.hpp"

#include <managers/keybinds.hpp>
#include <Geode/Geode.hpp>

using namespace geode::prelude;

Result<std::shared_ptr<SettingV3>> KeybindSetting::parse(std::string const& key, std::string const& modID, matjson::Value const& json) {
    auto res = std::make_shared<KeybindSetting>();
    auto root = checkJson(json, "KeybindSetting");

    res->init(key, modID, root);
    res->parseNameAndDescription(root);
    res->parseEnableIf(root);
    root.checkUnknownKeys();
    return root.ok(std::static_pointer_cast<SettingV3>(res));
}

bool KeybindSettingNode::init(std::shared_ptr<KeybindSetting> setting, float width) {
    if (!SettingNodeV3::init(setting, width))
        return false;

    m_buttonSprite = ButtonSprite::create("Keybind: 0", "goldFont.fnt", "GJ_button_01.png", .8f);
    if (this->getSetting()->getKey() == "voice-chat-keybind") {
        this->m_buttonSprite->setString(fmt::format("Keybind: {}", GlobedSettings::get().communication.voiceChatKey.get()).c_str());
    }
    else {
        this->m_buttonSprite->setString(fmt::format("Keybind: {}", GlobedSettings::get().communication.voiceDeafenKey.get()).c_str());
    }
    m_buttonSprite->setScale(.5f);
    m_button = CCMenuItemSpriteExtra::create(
        m_buttonSprite, this, menu_selector(KeybindSettingNode::onButton)
    );
    this->getButtonMenu()->addChildAtPosition(m_button, Anchor::Center);
    this->getButtonMenu()->setContentWidth(60);
    this->getButtonMenu()->updateLayout();

    this->updateState(nullptr);
    
    return true;
}

void KeybindSettingNode::onButton(CCObject*) {
    if (this->getSetting()->getKey() == "voice-chat-keybind") {
        this->m_buttonSprite->setString(fmt::format("Keybind: {}", GlobedSettings::get().communication.voiceChatKey.get()).c_str());
        auto kbRegisterLayer = KeybindRegisterLayer::create(0, this->m_buttonSprite);
        CCScene::get()->addChild(kbRegisterLayer);
    }
    else {
        this->m_buttonSprite->setString(fmt::format("Keybind: {}", GlobedSettings::get().communication.voiceDeafenKey.get()).c_str());
        auto kbRegisterLayer = KeybindRegisterLayer::create(1, this->m_buttonSprite);
        CCScene::get()->addChild(kbRegisterLayer);
    }
}

KeybindSettingNode* KeybindSettingNode::create(std::shared_ptr<KeybindSetting> setting, float width) {
    auto ret = new KeybindSettingNode();
    if (ret && ret->init(setting, width)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

SettingNodeV3* KeybindSetting::createNode(float width) {
    return KeybindSettingNode::create(
        std::static_pointer_cast<KeybindSetting>(shared_from_this()),
        width
    );
}