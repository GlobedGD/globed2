#include "KeybindSetupPopup.hpp"
#include <globed/core/KeybindsManager.hpp>

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

const CCSize KeybindSetupPopup::POPUP_SIZE { 240.f, 200.f };

bool KeybindSetupPopup::setup(enumKeyCodes key) {
    this->setTitle("Set Keybind");

    m_originalKey = key;

    m_keybindLabel = CCLabelBMFont::create("Keybind: None", "bigFont.fnt");
    m_mainLayer->addChildAtPosition(m_keybindLabel, Anchor::Center);

    auto applyButton = Build<ButtonSprite>::create("Apply", "bigFont.fnt", "GJ_button_01.png", 0.75f)
        .intoMenuItem([this] (auto) {
            if (m_originalKey == m_key) {
                this->removeFromParent();
                return;
            }

            auto& km = KeybindsManager::get();
            if (km.isKeyUsed(m_key)) {
                Notification::create("This key is already used by another action", NotificationIcon::Error, 1.0f)->show();
                return;
            }

            if (m_callback) {
                m_callback(m_key);
            }

            this->removeFromParent();
        })
        .id("keybind-apply-button")
        .pos(this->fromBottom(30.f))
        .intoNewParent(CCMenu::create())
        .pos(0.f, 0.f)
        .parent(m_mainLayer);

    this->keyDown(m_originalKey);

    return true;
}

void KeybindSetupPopup::keyDown(enumKeyCodes keyCode) {
    if (keyCode == KEY_Escape) {
        this->onClose(this);
        return;
    }

    // for some reason sometimes this just randomly gets pressed every frame??
    if (keyCode == -1) {
        return;
    }

    if (keyCode == KEY_Backspace) {
        this->keyDown(KEY_None);
        return;
    }

    m_key = keyCode;

    auto k = globed::convertCocosKey(keyCode);
    m_valid = k != globed::Key::None;

    if (m_valid) {
        m_keybindLabel->setString(fmt::format("Keybind: {}", globed::formatKey(k)).c_str());
        m_keybindLabel->setColor(ccColor3B{ 255, 255, 255 });
    } else {
        m_keybindLabel->setString("Keybind: None");
        m_keybindLabel->setColor(ccColor3B{ 216, 216, 216 });
    }

    m_keybindLabel->limitLabelWidth(POPUP_SIZE.width - 16.f, 0.75f, 0.5f);
}

void KeybindSetupPopup::setCallback(Callback&& cb) {
    m_callback = std::move(cb);
}

}