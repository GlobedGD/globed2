#include "server_list.hpp"

#include "server_list_cell.hpp"
#include "relay_tip_cell.hpp"
#include <net/manager.hpp>
#include <managers/game_server.hpp>
#include <managers/settings.hpp>
#include <managers/web.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

bool GlobedServerList::init() {
    if (!CCLayer::init()) return false;

    auto listview = Build<ListView>::create(CCArray::create(), 0.f, LIST_WIDTH, LIST_HEIGHT).collect();

    Build<GJListLayer>::create(listview, "Servers", globed::color::Brown, LIST_WIDTH, 220.f, 0)
        .zOrder(2)
        .anchorPoint(0.f, 0.f)
        .pos(0.f, 0.f)
        .parent(this)
        .store(bgListLayer);

    util::ui::makeListGray(bgListLayer);

    this->setContentSize(bgListLayer->getScaledContentSize());

    Build<ServerList>::create(LIST_WIDTH, LIST_HEIGHT - 2.f, globed::color::Brown)
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

    for (auto* cell : *listLayer) {
        if (auto slc = typeinfo_cast<ServerListCell*>(cell)) {
            auto server = gsm.getServer(slc->gsview.id);
            if (server.has_value()) {
                slc->updateWith(server.value());
            }
        }
    }
}

CCArray* GlobedServerList::createServerList() {
    auto ret = CCArray::create();

    auto& gs = GlobedSettings::get();
    auto& nm = NetworkManager::get();
    auto& gsm = GameServerManager::get();
    auto& wrm = WebRequestManager::get();

    for (const auto& [serverId, server] : gsm.getAllServers()) {
        auto cell = ServerListCell::create(server);
        ret->addObject(cell);
    }

    showingRelays = (gs.globed.showRelays || wrm.isRussian() || !gsm.getActiveRelayId().empty()) && gsm.hasRelays();
    if (showingRelays) {
        ret->addObject(RelayTipCell::create());
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
