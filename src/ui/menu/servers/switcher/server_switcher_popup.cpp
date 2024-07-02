#include "server_switcher_popup.hpp"

#include "server_cell.hpp"
#include "add_server_popup.hpp"
#include "direct_connect_popup.hpp"
#include <managers/central_server.hpp>
#include <util/collections.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

bool ServerSwitcherPopup::setup() {
    this->setID("ServerSwitcherPopup"_spr);

    this->setTitle("Server switcher");

    auto rlayout = util::ui::getPopupLayout(m_size);

    Build(ServerList::createForComments(LIST_WIDTH, LIST_HEIGHT, CentralServerListCell::CELL_HEIGHT))
        .anchorPoint(0.5f, 1.f)
        .pos(rlayout.fromTop(40.f))
        .store(listLayer)
        .parent(m_mainLayer);

    // buttons layout

    auto* menu = Build<CCMenu>::create()
        .layout(
            RowLayout::create()
                ->setAutoScale(true)
                ->setGap(5.0f)
                ->setAxisAlignment(AxisAlignment::Center)
        )
        .anchorPoint(0.5f, 0.5f)
        .pos(rlayout.fromBottom(30.f))
        .contentSize(LIST_WIDTH, 0.f)
        .parent(m_mainLayer)
        .id("server-switcher-btn-menu"_spr)
        .collect();

    // add new server button
    Build<ButtonSprite>::create("Add new", "bigFont.fnt", "GJ_button_01.png", 0.8f)
        .intoMenuItem([this](auto) {
            AddServerPopup::create(-1, this)->show();
        })
        .parent(menu)
        .id("server-switcher-btn-addserver"_spr);

    // direct connect button
    Build<ButtonSprite>::create("Direct connection", "bigFont.fnt", "GJ_button_01.png", 0.8f)
        .intoMenuItem([this](auto) {
            DirectConnectionPopup::create(this)->show();
        })
        .parent(menu)
        .id("server-switcher-btn-ipconnect"_spr);

    menu->updateLayout();

    this->reloadList();

    return true;
}

void ServerSwitcherPopup::reloadList() {
    auto cells = CCArray::create();

    auto& csm = CentralServerManager::get();
    auto servers = csm.getAllServers();

    for (size_t id = 0; id < servers.size(); id++) {
        auto& server = servers.at(id);
        auto* cell = CentralServerListCell::create(server, id, this);
        cells->addObject(cell);
    }

    listLayer->swapCells(cells);

    geode::cocos::handleTouchPriority(this);
}

void ServerSwitcherPopup::close() {
    this->onClose(this);
}

ServerSwitcherPopup* ServerSwitcherPopup::create() {
    auto ret = new ServerSwitcherPopup;

    if (ret->init(POPUP_WIDTH, POPUP_HEIGHT)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}