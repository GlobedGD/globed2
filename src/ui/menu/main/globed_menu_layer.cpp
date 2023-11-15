#include "globed_menu_layer.hpp"

#include <Geode/utils/web.hpp>
#include <UIBuilder.hpp>

#include "server_list_cell.hpp"
#include <util/ui.hpp>
#include <util/net.hpp>
#include <net/network_manager.hpp>
#include <managers/server_manager.hpp>
#include <managers/error_queues.hpp>

using namespace geode::prelude;

bool GlobedMenuLayer::init() {
    if (!CCLayer::init()) return false;

    auto listview = Build<ListView>::create(createServerList(), LIST_CELL_HEIGHT, LIST_WIDTH, LIST_HEIGHT)
        .collect();

    auto winsize = CCDirector::get()->getWinSize();

    Build<GJListLayer>::create(listview, "Servers", ccc4(0, 0, 0, 180), LIST_WIDTH, 220.f)
        .zOrder(2)
        .anchorPoint({0.f, 0.f})
        .parent(this)
        .store(listLayer);
    
    listLayer->setPosition({winsize / 2 - listLayer->getScaledContentSize() / 2});

    CCScheduler::get()->scheduleSelector(schedule_selector(GlobedMenuLayer::refreshServerList), this, 1.0f, false);

    Build<CCSprite>::createSpriteName("miniSkull_001.png")
        .scale(1.2f)
        .pos({25.f, 25.f})
        .intoMenuItem([this](CCObject*) {
            this->requestServerList();
        })
        .intoNewParent(CCMenu::create())
        .parent(this);

    util::ui::addBackground(this);

    auto menu = CCMenu::create();
    this->addChild(menu);

    util::ui::addBackButton(this, menu, util::ui::navigateBack);

    this->setKeyboardEnabled(true);
    this->setKeypadEnabled(true);

    return true;
}

CCArray* GlobedMenuLayer::createServerList() {
    auto ret = CCArray::create();

    auto& nm = NetworkManager::get();
    auto& sm = GlobedServerManager::get();
    
    bool authenticated = nm.authenticated();

    auto activeServer = sm.getActiveGameServer();

    for (const auto [serverId, server] : sm.extractGameServers()) {
        bool active = authenticated && serverId == activeServer;
        auto cell = ServerListCell::create(server);
        ret->addObject(cell);
    }

    return ret;
}

void GlobedMenuLayer::refreshServerList(float _) {
    auto& sm = GlobedServerManager::get();
    if (sm.pendingChanges.load(std::memory_order::relaxed)) {
        sm.pendingChanges.store(false, std::memory_order::relaxed);
        auto listview = Build<ListView>::create(createServerList(), LIST_CELL_HEIGHT, LIST_WIDTH, LIST_HEIGHT)
            .collect();

        listLayer->m_listView->removeFromParent();
        listLayer->m_listView = listview;
    }
}

void GlobedMenuLayer::requestServerList() {
    auto centralUrl = GlobedServerManager::get().getCentral();

    web::AsyncWebRequest()
        .userAgent(util::net::webUserAgent())
        .fetch(fmt::format("{}/servers", centralUrl))
        .json()
        .then([](json::Value response) {
            auto& sm = GlobedServerManager::get();
            sm.clearGameServers();
            sm.pendingChanges.store(true, std::memory_order::relaxed);

            try {
                auto serverList = response.as_array();
                for (const auto& obj : serverList) {
                    auto server = obj.as_object();
                    sm.addGameServer(
                        server["id"].as_string(),
                        server["name"].as_string(),
                        server["address"].as_string(),
                        server["region"].as_string()
                    );
                }
            } catch (const std::exception& e) {
                ErrorQueues::get().error("Failed to parse server list: <cy>{}</c>", e.what());
            }
        })
        .expect([](auto error) {
            ErrorQueues::get().error(fmt::format("Failed to fetch servers: <cy>{}</c>", error));

            auto& sm = GlobedServerManager::get();
            sm.clearGameServers();
            sm.pendingChanges.store(true, std::memory_order::relaxed);
        })
        .send();
}

void GlobedMenuLayer::keyBackClicked() {
    util::ui::navigateBack();
}