#include "menu_layer.hpp"

#include <managers/account.hpp>
#include <managers/central_server.hpp>
#include <managers/error_queues.hpp>
#include <managers/game_server.hpp>
#include <managers/settings.hpp>
#include <managers/friend_list.hpp>
#include <net/manager.hpp>
#include <ui/menu/main/globed_menu_layer.hpp>
#include <util/misc.hpp>
#include <util/net.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

bool HookedMenuLayer::init() {
    if (!MenuLayer::init()) return false;

    if (GJAccountManager::sharedState()->m_accountID > 0) {
        auto& flm = FriendListManager::get();
        flm.maybeLoad();
    }

    // auto connect
    util::misc::callOnce("menu-layer-init-autoconnect", []{
        if (!GlobedSettings::get().globed.autoconnect) return;

        auto& csm = CentralServerManager::get();
        auto& gsm = GameServerManager::get();
        auto& am = GlobedAccountManager::get();

        auto lastId = gsm.loadLastConnected();
        if (lastId.empty()) return;

        am.autoInitialize();

        if (csm.standalone()) {
            auto result = NetworkManager::get().connectStandalone();
            if (result.isErr()) {
                ErrorQueues::get().warn(fmt::format("Failed to connect: {}", result.unwrapErr()));
            }
        } else {
            auto cacheResult = gsm.loadFromCache();
            if (cacheResult.isErr()) {
                ErrorQueues::get().debugWarn(fmt::format("failed to autoconnect: {}", cacheResult.unwrapErr()));
                return;
            }

            auto lastServer = gsm.getServer(lastId);

            if (!lastServer.has_value()) {
                ErrorQueues::get().debugWarn("failed to autoconnect, game server not found");
                return;
            };

            if (!am.hasAuthKey()) {
                // no authkey, don't autoconnect
                return;
            }

            am.requestAuthToken(csm.getActive()->url, [lastServer] {
                auto result = NetworkManager::get().connect(lastServer.value());
                if (result.isErr()) {
                    ErrorQueues::get().warn(fmt::format("Failed to connect: {}", result.unwrapErr()));
                }
            });
        }
    });

    m_fields->btnActive = NetworkManager::get().established();
    this->updateGlobedButton();

    this->schedule(schedule_selector(HookedMenuLayer::maybeUpdateButton), 0.25f);

    return true;
}

void HookedMenuLayer::updateGlobedButton() {
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

    auto makeSprite = [this]{
        return CircleButtonSprite::createWithSpriteFrameName(
            "menuicon.png"_spr,
            1.f,
            m_fields->btnActive ? CircleBaseColor::Cyan : CircleBaseColor::Green,
            CircleBaseSize::MediumAlt
        );
    };

    if (!m_fields->globedBtn) {
        m_fields->globedBtn = Build<CircleButtonSprite>(makeSprite())
            .intoMenuItem(this, menu_selector(HookedMenuLayer::onGlobedButton))
            .zOrder(5) // force it to be at the end of the layout for consistency
            .id("main-menu-button"_spr)
            .parent(parent)
            .collect();
    } else {
        m_fields->globedBtn->setNormalImage(makeSprite());
    }

    parent->updateLayout();
}

void HookedMenuLayer::maybeUpdateButton(float) {
    bool authenticated = NetworkManager::get().established();
    if (authenticated != m_fields->btnActive) {
        m_fields->btnActive = authenticated;
        this->updateGlobedButton();
    }
}

void HookedMenuLayer::onGlobedButton(cocos2d::CCObject*) {
    auto accountId = GJAccountManager::sharedState()->m_accountID;
    if (accountId <= 0) {
        FLAlertLayer::create("Notice", "You need to be signed into a <cg>Geometry Dash account</c> in order to play online.", "Ok")->show();
        return;
    }

    util::ui::switchToScene(GlobedMenuLayer::create());
}

#if 0
#include <util/debug.hpp>
void HookedMenuLayer::onMoreGames(cocos2d::CCObject* s) {
    util::debug::Benchmarker bb;
    bb.runAndLog([&] {
        for (size_t i = 0; i < 1 * 1024 * 1024; i++) {
            auto x = typeinfo_cast<CCMenuItemSpriteExtra*>(s);
            if (!x) throw "wow";
        }
    }, "typeinfo");
}
#endif