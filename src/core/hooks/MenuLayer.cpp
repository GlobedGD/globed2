#include "MenuLayer.hpp"
#include <UIBuilder.hpp>
#include <globed/core/net/NetworkManager.hpp>
#include <globed/core/SettingsManager.hpp>
#include <ui/menu/GlobedMenuLayer.hpp>

using namespace geode::prelude;

namespace globed {

bool HookedMenuLayer::init() {
    if (!MenuLayer::init()) return false;

    this->recreateButton();

    return true;
}

void HookedMenuLayer::recreateButton() {
    CCNode* parent = nullptr;
    CCPoint pos;

    if ((parent = this->getChildByID("bottom-menu"))) {
        pos = CCPoint{0.f, 0.f};
    } else if ((parent = this->getChildByID("side-menu"))) {
        pos = CCPoint{0.f, 0.f};
    } else if ((parent = this->getChildByID("right-side-menu"))) {
        pos = CCPoint{0.f, 0.f};
    }

    if (!parent) {
        log::warn("failed to position the globed button");
        return;
    }

    auto makeSprite = [this]() -> CCNode* {
        if (false) {
            return CCSprite::createWithSpriteFrameName("GJ_reportBtn_001.png");
        } else {
            return CircleButtonSprite::createWithSprite(
                "globed-gold-icon.png"_spr,
                1.f,
                m_fields->btnActive ? CircleBaseColor::Cyan : CircleBaseColor::Green,
                CircleBaseSize::MediumAlt
            );
        }
    };

    if (!m_fields->btn) {
        m_fields->btn = Build<CCNode>(makeSprite())
            .intoMenuItem(this, menu_selector(HookedMenuLayer::onGlobedButton))
            .zOrder(5) // force it to be at the end of the layout for consistency
            .id("main-menu-button"_spr)
            .parent(parent)
            .collect();
    } else {
        m_fields->btn->setNormalImage(makeSprite());
    }

    parent->updateLayout();
}

void HookedMenuLayer::onGlobedButton(cocos2d::CCObject*) {
    if (!globed::value<bool>("core.flags.seen-consent-notice").value_or(false)) {
        // TODO
        createQuickPopup(
            "Note",
            "This is where I show the consent popup and yap about account verification but I don't have the time to write this rn",
            "Cancel",
            "Ok",
            [this](auto, bool yup) {
                if (yup) {
                    globed::setValue("core.flags.seen-consent-notice", true);
                    this->onGlobedButton(nullptr);
                }
            }
        );

        return;
    }

    GlobedMenuLayer::create()->switchTo();
}

}
