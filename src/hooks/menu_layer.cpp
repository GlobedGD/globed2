#include "menu_layer.hpp"

#include <managers/account.hpp>
#include <managers/central_server.hpp>
#include <managers/error_queues.hpp>
#include <managers/game_server.hpp>
#include <managers/settings.hpp>
#include <net/network_manager.hpp>
#include <ui/menu/main/globed_menu_layer.hpp>
#include <util/misc.hpp>
#include <util/net.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

bool HookedMenuLayer::init() {
    if (!MenuLayer::init()) return false;

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

            auto result = am.generateAuthCode();
            if (result.isErr()) {
                // invalid authkey? clear it so the user can relog. happens if user changes their password
                ErrorQueues::get().debugWarn(fmt::format(
                    "Failed to generate authcode: {}",
                    result.unwrapErr()
                ));
                am.clearAuthKey();
                return;
            }

            std::string authcode = result.unwrap();

            auto gdData = am.gdData.lock();
            am.requestAuthToken(csm.getActive()->url, gdData->accountId, gdData->accountName, authcode, [lastServer] {
                auto result = NetworkManager::get().connectWithView(lastServer.value());
                if (result.isErr()) {
                    ErrorQueues::get().warn(fmt::format("Failed to connect: {}", result.unwrapErr()));
                }
            });
        }
    });

    m_fields->btnActive = NetworkManager::get().established();
    this->updateGlobedButton();

    CCScheduler::get()->scheduleSelector(schedule_selector(HookedMenuLayer::maybeUpdateButton), this, 0.25f, false);

    return true;
}

void HookedMenuLayer::updateGlobedButton() {
    if (m_fields->globedBtn) m_fields->globedBtn->removeFromParent();

    auto menu = this->getChildByID("bottom-menu");

    Build<CircleButtonSprite>(CircleButtonSprite::createWithSpriteFrameName(
        "menuicon.png"_spr,
        1.f,
        m_fields->btnActive ? CircleBaseColor::Cyan : CircleBaseColor::Green,
        CircleBaseSize::MediumAlt
        ))
        .intoMenuItem([](auto) {
            auto accountId = GJAccountManager::sharedState()->m_accountID;
            if (accountId <= 0) {
                FLAlertLayer::create("Notice", "You need to be signed into a <cg>Geometry Dash account</c> in order to play online.", "Ok")->show();
                return;
            }

            util::ui::switchToScene(GlobedMenuLayer::create());
        })
        .id("main-menu-button"_spr)
        .parent(menu)
        .store(m_fields->globedBtn);

    menu->updateLayout();
}

void HookedMenuLayer::maybeUpdateButton(float) {
    bool authenticated = NetworkManager::get().established();
    if (authenticated != m_fields->btnActive) {
        m_fields->btnActive = authenticated;
        this->updateGlobedButton();
    }
}