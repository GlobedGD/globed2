#include "server_list.hpp"

#include "server_list_cell.hpp"
#include <net/manager.hpp>
#include <managers/game_server.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

bool GlobedServerList::init() {
    if (!CCLayer::init()) return false;

    auto listview = Build<ListView>::create(CCArray::create(), 0.f, LIST_WIDTH, LIST_HEIGHT).collect();

    listLayer = Build<GJListLayer>::create(listview, "Servers", util::ui::BG_COLOR_BROWN, LIST_WIDTH, 220.f, 0)
        .zOrder(2)
        .anchorPoint(0.f, 0.f)
        .pos(0.f, 0.f)
        .parent(this)
        .collect();

    this->setContentSize(listLayer->getScaledContentSize());

    return true;
}

void GlobedServerList::forceRefresh() {
    listLayer->m_listView->removeFromParent();

    auto serverList = createServerList();
    listLayer->m_listView = Build<ListView>::create(serverList, ServerListCell::CELL_HEIGHT, LIST_WIDTH, LIST_HEIGHT)
        .parent(listLayer)
        .collect();
}

void GlobedServerList::softRefresh() {
    auto& gsm = GameServerManager::get();

    auto listCells = listLayer->m_listView->m_tableView->m_contentLayer->getChildren();
    if (listCells == nullptr) {
        return;
    }

    auto active = gsm.getActiveId();

    bool authenticated = NetworkManager::get().established();

    for (auto* obj : CCArrayExt<CCNode*>(listCells)) {
        auto slc = static_cast<ServerListCell*>(obj->getChildren()->objectAtIndex(2));
        auto server = gsm.getServer(slc->gsview.id);
        if (server.has_value()) {
            slc->updateWith(server.value(), authenticated && slc->gsview.id == active);
        }
    }
}

CCArray* GlobedServerList::createServerList() {
    auto ret = CCArray::create();

    auto& nm = NetworkManager::get();
    auto& gsm = GameServerManager::get();

    bool authenticated = nm.established();

    auto activeServer = gsm.getActiveId();

    for (const auto& [serverId, server] : gsm.getAllServers()) {
        bool active = authenticated && serverId == activeServer;
        auto cell = ServerListCell::create(server, active);
        ret->addObject(cell);
    }

    return ret;
}

GlobedServerList* GlobedServerList::create() {
    auto ret = new GlobedServerList();
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}
