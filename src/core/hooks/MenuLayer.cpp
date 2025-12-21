#include "MenuLayer.hpp"
#include <UIBuilder.hpp>
#include <globed/core/net/NetworkManager.hpp>
#include <globed/core/SettingsManager.hpp>
#include <globed/core/ServerManager.hpp>
#include <ui/menu/GlobedMenuLayer.hpp>
#include <ui/menu/ConsentPopup.hpp>

#include <argon/argon.hpp>

using namespace geode::prelude;

namespace globed {

static void initiateAutoConnect() {
    // check if signed into an account
    if (!argon::signedIn() || !globed::flag("core.flags.seen-consent-notice")) {
        return;
    }

    if (globed::flag("core.was-connected")) {
        auto& sm = ServerManager::get();
        auto url = sm.getActiveServer().url;

        if (auto err = NetworkManager::get().connectCentral(url).err()) {
            log::error("failed to autoconnect: {}", err);
            return;
        }
    }
}

bool HookedMenuLayer::init() {
    if (!MenuLayer::init()) return false;

    argon::initConfigLock();

    this->recreateButton();

    static bool invoked = false;
    if (!invoked) {
        invoked = true;

        if (globed::setting<bool>("core.autoconnect")) {
            initiateAutoConnect();
        }
    }

    this->schedule(schedule_selector(HookedMenuLayer::checkButton), 0.1f);

    return true;
}

void HookedMenuLayer::checkButton(float) {
    auto& fields = *m_fields.self();

    bool active = NetworkManager::get().getConnectionState() == ConnectionState::Connected;

    if (active == fields.btnActive) return;
    fields.btnActive = active;

    this->recreateButton();
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
    if (globed::flag("core.flags.seen-consent-notice")) {
        GlobedMenuLayer::create()->switchTo();
        return;
    }

    auto p = ConsentPopup::create();
    p->setCallback([](bool accepted) {
        globed::setValue("core.flags.seen-consent-notice", accepted);

        if (accepted) {
            GlobedMenuLayer::create()->switchTo();
        }
    });
    p->show();
}

}
