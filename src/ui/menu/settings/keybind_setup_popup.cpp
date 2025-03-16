#include "keybind_setup_popup.hpp"
#include "Geode/cocos/robtop/keyboard_dispatcher/CCKeyboardDelegate.h"
#include "managers/keybinds.hpp"

#include <util/ui.hpp>

using namespace geode::prelude;

bool KeybindSetupPopup::setup(void* settingStorage) {
    this->setTitle("Set Keybind");

    m_keybindLabel = CCLabelBMFont::create("Keybind: None", "bigFont.fnt");
    m_mainLayer->addChildAtPosition(m_keybindLabel, Anchor::Center);

    auto& gs = GlobedSettings::get();

    int keybind = *(int*)(settingStorage);
    this->settingStorage = (int*)settingStorage;

    auto applyButton = Build<ButtonSprite>::create("Apply", "bigFont.fnt", "GJ_button_01.png", 0.75f)
        .intoMenuItem([&gs, this] (auto) {
            if (!this->isValid) {
                Notification::create("Please choose a valid key", NotificationIcon::Error, 0.5f)->show();
                return;
            }

            *this->settingStorage = this->key;
            gs.save();

            auto keystr = globed::formatKey((enumKeyCodes) this->key);
            Notification::create(fmt::format("Set Keybind to {}", keystr), NotificationIcon::Success, 0.5f)->show();

            this->removeFromParent();
        })
        .id("keybind-apply-button")
        .pos(util::ui::getPopupLayoutAnchored(m_size).fromBottom(30.f))
        .intoNewParent(CCMenu::create())
        .pos(0.f, 0.f)
        .parent(m_mainLayer);

    this->keyDown((enumKeyCodes)*this->settingStorage);

    return true;
}

void KeybindSetupPopup::keyDown(enumKeyCodes keyCode) {
    if (keyCode == KEY_Escape) {
        this->onClose(this);
        return;
    }

    this->key = (int)keyCode;

    auto k = KeybindsManager::convertCocosKey(keyCode);
    this->isValid = k != globed::Key::None;

    if (this->isValid) {
        m_keybindLabel->setString(fmt::format("Keybind: {}", globed::formatKey(k)).c_str());
        m_keybindLabel->setColor(ccColor3B{ 255, 255, 255 });
    } else {
        m_keybindLabel->setString("Invalid key");
        m_keybindLabel->setColor(ccColor3B{ 224, 75, 16 });
    }

    m_keybindLabel->limitLabelWidth(POPUP_WIDTH - 16.f, 0.75f, 0.5f);
}

KeybindSetupPopup* KeybindSetupPopup::create(void* settingStorage) {
    auto ret = new KeybindSetupPopup();
    if (ret->initAnchored(POPUP_WIDTH, POPUP_HEIGHT, settingStorage)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}
