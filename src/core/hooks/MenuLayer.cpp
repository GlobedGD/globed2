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

static void showSelfDisabled();

static bool shouldDisableSelf() {
    return Loader::get()->isModLoaded("weebify.separate_dual_icons");
}

static void initiateAutoConnect() {
    // check if signed into an account
    if (!argon::signedIn() || !globed::flag("core.flags.seen-consent-notice") || shouldDisableSelf()) {
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
    this->recreateButton();
}

void HookedMenuLayer::recreateButton() {
    auto& fields = *m_fields.self();

    bool active = NetworkManager::get().getConnectionState() == ConnectionState::Connected;

    if (active == fields.btnActive) return;
    fields.btnActive = active;

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

    auto makeSprite = [&]() -> CCNode* {
        CCNode* spr = CircleButtonSprite::createWithSprite(
            "globed-gold-icon.png"_spr,
            1.f,
            active ? CircleBaseColor::Cyan : CircleBaseColor::Green,
            CircleBaseSize::MediumAlt
        );

        if (!spr) {
            spr = CCSprite::createWithSpriteFrameName("GJ_reportBtn_001.png");
        }

        return spr;
    };

    if (!fields.btn) {
        fields.btn = Build<CCNode>(makeSprite())
            .intoMenuItem(this, menu_selector(HookedMenuLayer::onGlobedButton))
            .zOrder(5) // force it to be at the end of the layout for consistency
            .id("main-menu-button"_spr)
            .parent(parent)
            .collect();
    } else {
        fields.btn->setNormalImage(makeSprite());
    }

    parent->updateLayout();
}

void HookedMenuLayer::onGlobedButton(cocos2d::CCObject*) {
    if (shouldDisableSelf()) {
        showSelfDisabled();
        return;
    }

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

void showSelfDisabled() {
    globed::confirmPopup(
        "Incompatible Mod Detected",
        "The mod <cy>Separate Dual Icons</c> is currently <cr>incompatible</c> with Globed and will cause <cr>crashes</c> if both are enabled. You must disable one of the mods to continue.\n\n"
        "Do you want to disable <cy>Separate Dual Icons</c> and restart the game?",
        "Cancel",
        "Restart",
        [](auto) {
            auto mod = Loader::get()->getLoadedMod("weebify.separate_dual_icons");
            if (!mod) return;

            (void) mod->disable();
            utils::game::restart(true);
        }
    );
}

}
