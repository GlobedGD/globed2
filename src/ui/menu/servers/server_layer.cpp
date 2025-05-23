#include "server_layer.hpp"

#include <matjson/reflect.hpp>
#include <matjson/stl_serialize.hpp>

#include "switcher/server_switcher_popup.hpp"
#include <ui/menu/main/kofi_popup.hpp>
#include <ui/menu/credits/credits_popup.hpp>
#include <data/packets/server/connection.hpp>
#include <managers/account.hpp>
#include <managers/central_server.hpp>
#include <managers/game_server.hpp>
#include <managers/error_queues.hpp>
#include <managers/settings.hpp>
#include <managers/friend_list.hpp>
#include <ui/menu/main/globed_menu_layer.hpp>
#include <ui/menu/settings/settings_layer.hpp>
#include <net/manager.hpp>
#include <util/ui.hpp>
#include <util/gd.hpp>
#include <util/format.hpp>

using namespace geode::prelude;

bool GlobedServersLayer::init() {
    if (!CCLayer::init()) return false;

    auto& settings = GlobedSettings::get();
    prevShownRelays = settings.globed.showRelays;

    this->setID("GlobedServersLayer"_spr);

    util::ui::prepareLayer(this, false);

    auto winSize = CCDirector::get()->getWinSize();

    Build(util::ui::makeRepeatingBackground("game_bg_01_001.png", {40, 40, 40}))
        .id("background")
        .parent(this)
        .store(this->background);

    GlobedAccountManager::get().autoInitialize();

    auto* buttonMenu = Build<CCMenu>::create()
        .layout(ColumnLayout::create()->setAutoScale(true)->setAxisAlignment(AxisAlignment::Start))
        .anchorPoint(0.f, 0.f)
        .pos(15.f, 20.f)
        .id("left-side-menu")
        .parent(this)
        .collect();

    // central server switcher
    Build<CCSprite>::createSpriteName("icon-server-folder.png"_spr)
        .with([&](auto* spr) {
            util::ui::rescaleToMatch(spr, {43.f, 41.5f});
        })
        .intoMenuItem([this] {
            if (auto* popup = ServerSwitcherPopup::create()) {
                popup->m_noElasticity = true;
                popup->show();
            }
        })
        .id("btn-server-switcher")
        .scaleMult(1.15f)
        .parent(buttonMenu);

    // settings button
    Build<CCSprite>::createSpriteName("icon-settings.png"_spr)
        .with([&](auto* spr) {
            util::ui::rescaleToMatch(spr, {43.f, 41.5f});
        })
        .intoMenuItem([](auto) {
            util::ui::switchToScene(GlobedSettingsLayer::create());
        })
        .scaleMult(1.15f)
        .id("btn-settings")
        .parent(buttonMenu);

    buttonMenu->updateLayout();

    // server list
    Build<GlobedServerList>::create()
        .zOrder(2)
        .anchorPoint(0.f, 0.f)
        .parent(this)
        .id("server-list")
        .store(serverList);

    if (util::ui::getAspectRatio() < 1.6f) {
        serverList->setScaleX(0.9f);
    }

    serverList->setPosition({winSize / 2 - serverList->getScaledContentSize() / 2});

    // right button menu
    auto* rightButtonMenu = Build<CCMenu>::create()
        .layout(
            ColumnLayout::create()
                ->setAutoScale(true)
                ->setGap(4.f)
                ->setAxisAlignment(AxisAlignment::Start)
        )
        .anchorPoint(1.f, 0.f)
        .pos(winSize.width - 15.f, 20.f)
        .parent(this)
        .id("right-button-menu")
        .collect();

    // kofi button
    CCMenuItemSpriteExtra* kofi = Build<CCSprite>::createSpriteName("icon-kofi.png"_spr)
        .intoMenuItem([](auto) {
            GlobedKofiPopup::create()->show();
        })
        .scaleMult(1.15f)
        .id("btn-kofi")
        .parent(rightButtonMenu);

    // kofi glow
    CCSprite* glow = Build<CCSprite>::createSpriteName("icon-glow.png"_spr)
        .id("btn-kofi-glow"_spr)
        .pos(kofi->getScaledContentSize() / 2)
        .zOrder(-1)
        .color({255, 255, 0})
        .opacity(200)
        .scale(0.93f)
        .blendFunc({GL_ONE, GL_ONE})
        .parent(kofi);

    glow->runAction(CCRepeatForever::create(CCSequence::create(CCFadeTo::create(1.5f, 50), CCFadeTo::create(1.5f, 200), nullptr)));

    // credits button
    Build<CCSprite>::createSpriteName("icon-credits.png"_spr)
        .intoMenuItem([](auto) {
            auto* popup = GlobedCreditsPopup::create();
            popup->m_noElasticity = true;
            popup->show();
        })
        .scaleMult(1.15f)
        .id("btn-credits")
        .parent(rightButtonMenu);

    rightButtonMenu->updateLayout();

    // initialize flm here, which should give us enough time to load friend list until a connection is made
    auto& flm = FriendListManager::get();
    flm.maybeLoad();

    this->schedule(schedule_selector(GlobedServersLayer::updateServerList), 0.1f);
    this->schedule(schedule_selector(GlobedServersLayer::pingServers), 5.0f);

    this->updateServerList(0.f);

    initializing = false;

    return true;
}

void GlobedServersLayer::keyBackClicked() {
    util::ui::navigateBack();
}

void GlobedServersLayer::onExit() {
    CCLayer::onExit();
    this->cancelWebRequest();
}

void GlobedServersLayer::transitionToMainLayer() {
    util::ui::replaceScene(GlobedMenuLayer::create());
}

void GlobedServersLayer::updateServerList(float) {
    auto& am = GlobedAccountManager::get();
    auto& csm = CentralServerManager::get();
    auto& nm = NetworkManager::get();

    // if we are connected to a server, navigate away
    if (nm.established() && !typeinfo_cast<CCTransitionScene*>(CCScene::get()) && !transitioningAway) {
        auto* seq = CCSequence::create(
            CCDelayTime::create(0.2f),
            CCCallFunc::create(this, callfunc_selector(GlobedServersLayer::transitionToMainLayer)),
            nullptr
        );
        this->runAction(seq);
        transitioningAway = true;

        return;
    }

    // if we are logged out of our account, navigate away
    if (GJAccountManager::get()->m_accountID <= 0 && !typeinfo_cast<CCTransitionScene*>(CCScene::get()) && !transitioningAway) {
        util::gd::safePopScene();
        transitioningAway = true;

        return;
    }

    // if we recently switched a central server, redo everything
    if ((csm.recentlySwitched || initializing || !serversLoaded) && serversLoadingFor.value_or(-4) != csm.getActiveIndex()) {
        serversLoadingFor = csm.getActiveIndex();
        csm.recentlySwitched = false;
        this->cancelWebRequest();
        this->requestServerList();
    }

    // if there are pending changes, hard refresh the list and ping all servers
    auto& gsm = GameServerManager::get();
    if (gsm.pendingChanges || initializing) {
        gsm.pendingChanges = false;

        serverList->forceRefresh();

        this->pingServers(0.f);

        return;
    }

    auto& settings = GlobedSettings::get();
    if (settings.globed.showRelays != prevShownRelays) {
        prevShownRelays = settings.globed.showRelays;
        serverList->forceRefresh();

        return;
    }

    // if there were no pending changes, still update the server data (ping, players, etc.)
    serverList->softRefresh();
}

void GlobedServersLayer::requestServerList() {
    this->cancelWebRequest();

    auto& csm = CentralServerManager::get();

    if (csm.standalone()) {
        return;
    }

    auto centralUrl = csm.getActive();

    if (!centralUrl) {
        return;
    }

    auto request = WebRequestManager::get().fetchServerMeta();

    requestListener.bind(this, &GlobedServersLayer::requestCallback);
    requestListener.setFilter(std::move(request));
}

void GlobedServersLayer::requestCallback(typename WebRequestManager::Event* event) {
    if (!event || !event->getValue()) return;

    auto& csm = CentralServerManager::get();
    auto& gsm = GameServerManager::get();

    serversLoadingFor = std::nullopt;
    serversLoaded = true;

    auto result = std::move(*event->getValue());

    if (!result.ok()) {
        gsm.clear();
        csm.updateCacheForActive("");

        ErrorQueues::get().error(fmt::format("Failed to fetch servers.\n\nReason: <cy>{}</c>", result.getError()));

        return;
    }

    auto response = result.text().unwrapOrDefault();
    auto res = matjson::parseAs<MetaResponse>(response);

    if (!res) {
        gsm.clear();
        csm.updateCacheForActive("");
        log::warn("failed to parse server list: {}", res.unwrapErr());
        log::warn("{}", response);
        ErrorQueues::get().error(fmt::format("Failed to parse server list: <cy>{}</c>", res.unwrapErr()));
        return;
    }

    csm.updateCacheForActive(response);
    csm.initFromMeta(res.unwrap());
}

void GlobedServersLayer::cancelWebRequest() {
    requestListener.getFilter().cancel();
}

void GlobedServersLayer::pingServers(float d) {
    auto& nm = NetworkManager::get();
    nm.pingServers();
}

GlobedServersLayer* GlobedServersLayer::create() {
    auto ret = new GlobedServersLayer();
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}