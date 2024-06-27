#include "server_list.hpp"

#include "server_list_cell.hpp"
#include <net/manager.hpp>
#include <managers/game_server.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

bool GlobedServerList::init() {
    if (!CCLayer::init()) return false;

    auto listview = Build<ListView>::create(CCArray::create(), 0.f, LIST_WIDTH, LIST_HEIGHT).collect();

    Build<GJListLayer>::create(listview, "Servers", util::ui::BG_COLOR_BROWN, LIST_WIDTH, 220.f, 0)
        .zOrder(2)
        .anchorPoint(0.f, 0.f)
        .pos(0.f, 0.f)
        .parent(this)
        .store(bgListLayer);

    util::ui::makeListGray(bgListLayer);

    this->setContentSize(bgListLayer->getScaledContentSize());

    Build<ServerList>::create(LIST_WIDTH, LIST_HEIGHT - 5.f, util::ui::BG_COLOR_BROWN, ServerListCell::CELL_HEIGHT)
        .zOrder(3)
        .anchorPoint(0.5f, 0.5f)
        .pos(bgListLayer->getScaledContentSize() / 2.f)
        .parent(bgListLayer)
        .store(listLayer);

    listLayer->disableOverScrollUp(); // does nothing for now
    listLayer->setCellColor(ccColor3B{90, 90, 90});

    return true;
}

void GlobedServerList::forceRefresh() {
    auto serverList = createServerList();
    listLayer->swapCells(this->createServerList());
    listLayer->scrollToTop();
}

void GlobedServerList::softRefresh() {
    auto& gsm = GameServerManager::get();

    auto active = gsm.getActiveId();

    bool authenticated = NetworkManager::get().established();

    for (auto* slc : *listLayer) {
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
