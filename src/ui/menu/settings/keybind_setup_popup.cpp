#include "keybind_setup_popup.hpp"
#include <managers/keybinds.hpp>

#include <util/ui.hpp>

using namespace geode::prelude;

bool KeybindSetupPopup::setup(enumKeyCodes initialKey, Callback callback) {
    this->setTitle("Set Keybind");

    this->originalKey = initialKey;
    this->callback = std::move(callback);

    m_keybindLabel = CCLabelBMFont::create("Keybind: None", "bigFont.fnt");
    m_mainLayer->addChildAtPosition(m_keybindLabel, Anchor::Center);

    auto& gs = GlobedSettings::get();

    auto applyButton = Build<ButtonSprite>::create("Apply", "bigFont.fnt", "GJ_button_01.png", 0.75f)
        .intoMenuItem([&gs, this] (auto) {
            if (this->originalKey == this->key) {
                this->removeFromParent();
                return;
            }

            auto& km = KeybindsManager::get();
            if (this->key != KEY_None && km.isKeyUsed(this->key)) {
                Notification::create("This key is already used by another action", NotificationIcon::Error, 1.0f)->show();
                return;
            }

            if (this->callback) {
                this->callback(this->key);
            }

            this->removeFromParent();
        })
        .id("keybind-apply-button")
        .pos(util::ui::getPopupLayoutAnchored(m_size).fromBottom(30.f))
        .intoNewParent(CCMenu::create())
        .pos(0.f, 0.f)
        .parent(m_mainLayer);

    this->keyDown(initialKey);

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

    this->key = keyCode;

    auto k = KeybindsManager::convertCocosKey(keyCode);
    this->isValid = k != globed::Key::None;

    if (this->isValid) {
        m_keybindLabel->setString(fmt::format("Keybind: {}", globed::formatKey(k)).c_str());
        m_keybindLabel->setColor(ccColor3B{ 255, 255, 255 });
    } else {
        m_keybindLabel->setString("Keybind: None");
        m_keybindLabel->setColor(ccColor3B{ 216, 216, 216 });
    }

    m_keybindLabel->limitLabelWidth(POPUP_WIDTH - 16.f, 0.75f, 0.5f);
}

KeybindSetupPopup* KeybindSetupPopup::create(enumKeyCodes initialKey, Callback callback) {
    auto ret = new KeybindSetupPopup();
    if (ret->initAnchored(POPUP_WIDTH, POPUP_HEIGHT, initialKey, std::move(callback))) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}
