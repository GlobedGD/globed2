#include "keybind_setup_popup.hpp"

#include <util/ui.hpp>

using namespace geode::prelude;

bool KeybindSetupPopup::setup(int key, globed::Keybinds keybind) {
    this->setTitle("Set Keybind");

    m_keybindLabel = CCLabelBMFont::create("Keybind: None", "bigFont.fnt");
    m_keybindLabel->setScale(0.75f);
    m_keybindLabel->limitLabelWidth(POPUP_WIDTH - 16.f, 0.75f, 0.5f);
    m_mainLayer->addChildAtPosition(m_keybindLabel, Anchor::Center);

    auto& gs = GlobedSettings::get();

    auto applyButton = Build<ButtonSprite>::create("Apply", "bigFont.fnt", "GJ_button_01.png", 0.75f)
        .intoMenuItem([&gs, keybind, this] (auto) {
            switch (keybind) {
                case globed::Keybinds::VoiceChatKey: {
                    gs.communication.voiceChatKey = this->key;
                    break;
                }
                case globed::Keybinds::VoiceDeafenKey: {
                    gs.communication.voiceDeafenKey = this->key;
                    break;
                }
                default: break;
            }

            Notification::create(fmt::format("Set Keybind to {}", globed::formatKey((enumKeyCodes)this->key)), NotificationIcon::Success, 0.5f)->show();

            this->removeFromParent();
        })
        .id("keybind-apply-button")
        .pos(util::ui::getPopupLayoutAnchored(m_size).fromBottom(30.f))
        .intoNewParent(CCMenu::create())
        .pos(0.f, 0.f)
        .parent(m_mainLayer);

    switch (keybind) {
        case globed::Keybinds::VoiceChatKey: {
            this->keyDown((enumKeyCodes)gs.communication.voiceChatKey.get());
            break;
        }
        case globed::Keybinds::VoiceDeafenKey: {
            this->keyDown((enumKeyCodes)gs.communication.voiceDeafenKey.get());
            break;
        }
        default: break;
    }

    return true;
}

void KeybindSetupPopup::keyDown(enumKeyCodes keyCode) {
    this->key = (int)keyCode;

    m_keybindLabel->setString(fmt::format("Keybind: {}", globed::formatKey(keyCode)).c_str());
}

KeybindSetupPopup* KeybindSetupPopup::create(int key, globed::Keybinds keybind) {
    auto ret = new KeybindSetupPopup();
    if (ret->initAnchored(POPUP_WIDTH, POPUP_HEIGHT, key, keybind)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}