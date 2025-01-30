#include "keybind_setting.hpp"

#include <managers/keybinds.hpp>

using namespace geode::prelude;

void KeybindSettingNode::onButton(CCObject*) {
    
}

KeybindSettingNode* KeybindSettingNode::create(std::shared_ptr<KeybindSetting> setting, float width) {
    auto ret = new KeybindSettingNode();
    if (ret && ret->init(setting, width)) {
        ret->autorelese();
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