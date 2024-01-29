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

        log::debug("last connected server: {}", lastId);

        am.autoInitialize();

        if (csm.standalone()) {
            auto result = NetworkManager::get().connectStandalone();
            if (result.isErr()) {
                ErrorQueues::get().warn(fmt::format("Failed to connect: {}", result.unwrapErr()));
            }
        } else {
            auto lastServer = gsm.getServer(lastId);
            if (!lastServer.has_value()) return;

            std::string authcode;
            try {
                authcode = am.generateAuthCode();
            } catch (const std::exception& e) {
                // invalid authkey? clear it so the user can relog. happens if user changes their password
                ErrorQueues::get().debugWarn(fmt::format(
                    "Failed to generate authcode: {}",
                    e.what()
                ));
                am.clearAuthKey();
                return;
            }

            auto gdData = am.gdData.lock();
            am.requestAuthToken(csm.getActive()->url, gdData->accountId, gdData->accountName, authcode, [lastServer] {
                auto result = NetworkManager::get().connectWithView(lastServer.value());
                if (result.isErr()) {
                    ErrorQueues::get().warn(fmt::format("Failed to connect: {}", result.unwrapErr()));
                }
            });
        }
    });

    this->updateGlobedButton();

    CCScheduler::get()->scheduleSelector(schedule_selector(HookedMenuLayer::maybeUpdateButton), this, 0.1f, false);

    return true;
}

void HookedMenuLayer::updateGlobedButton() {
    if (m_fields->globedBtn) m_fields->globedBtn->removeFromParent();

    auto menu = this->getChildByID("bottom-menu");

    Build<CircleButtonSprite>(CircleButtonSprite::createWithSpriteFrameName(
        "miniSkull_001.png",
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

            CCDirector::get()->pushScene(CCTransitionFade::create(0.5f, GlobedMenuLayer::scene()));
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