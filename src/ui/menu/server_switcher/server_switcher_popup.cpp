#include "server_switcher_popup.hpp"

#include "server_cell.hpp"
#include "add_server_popup.hpp"
#include "direct_connect_popup.hpp"
#include <managers/central_server.hpp>
#include <util/collections.hpp>

using namespace geode::prelude;

bool ServerSwitcherPopup::setup() {
    this->setTitle("Server switcher");

    auto listview = ListView::create(CCArray::create(), CentralServerListCell::CELL_HEIGHT, LIST_WIDTH, LIST_HEIGHT);
    listLayer = GJCommentListLayer::create(listview, "", {192, 114, 62, 255}, LIST_WIDTH, LIST_HEIGHT, false);

    float xpos = (m_mainLayer->getScaledContentSize().width - LIST_WIDTH) / 2;
    listLayer->setPosition({xpos, 70.f});
    m_mainLayer->addChild(listLayer);

    // buttons layout

    auto* menu = Build<CCMenu>::create()
        .layout(
            RowLayout::create()
                ->setAutoScale(true)
                ->setGap(5.0f)
                ->setAxisAlignment(AxisAlignment::Center)
        )
        .anchorPoint(0.5f, 0.5f)
        .pos(m_mainLayer->getScaledContentSize().width / 2, 50.f)
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

    listLayer->m_list->removeFromParent();
    listLayer->m_list = Build<ListView>::create(cells, CentralServerListCell::CELL_HEIGHT, LIST_WIDTH, LIST_HEIGHT)
        .parent(listLayer)
        .collect();

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