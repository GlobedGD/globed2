#include "room_popup.hpp"

#include <UIBuilder.hpp>

#include "player_list_cell.hpp"
#include <data/packets/all.hpp>
#include <net/network_manager.hpp>
#include <managers/error_queues.hpp>
#include <managers/room.hpp>

using namespace geode::prelude;

bool RoomPopup::setup() {
    auto& nm = NetworkManager::get();
    if (!nm.established()) {
        return false;
    }

    auto& rm = RoomManager::get();

    nm.addListener<RoomPlayerListPacket>([this](RoomPlayerListPacket* packet) {
        this->playerList = packet->data;
        RoomManager::get().setRoom(packet->roomId);
        this->onLoaded();
    });

    auto listview = ListView::create(CCArray::create(), PlayerListCell::CELL_HEIGHT, LIST_WIDTH, LIST_HEIGHT);
    listLayer = GJCommentListLayer::create(listview, "", {192, 114, 62, 255}, LIST_WIDTH, LIST_HEIGHT, false);

    float xpos = (m_mainLayer->getScaledContentSize().width - LIST_WIDTH) / 2;
    listLayer->setPosition({xpos, 50.f});
    m_mainLayer->addChild(listLayer);

    this->reloadPlayerList();

    return true;
}

void RoomPopup::onLoaded() {
    this->removeLoadingCircle();

    auto cells = CCArray::create();

    for (const auto& pdata : playerList) {
        auto* cell = PlayerListCell::create(pdata);
        cells->addObject(cell);
    }

    listLayer->m_list->removeFromParent();
    listLayer->m_list = Build<ListView>::create(cells, PlayerListCell::CELL_HEIGHT, LIST_WIDTH, LIST_HEIGHT)
        .parent(listLayer)
        .collect();

    auto& rm = RoomManager::get();
    this->setTitle(rm.isInGlobal() ? "Global room" : fmt::format("Room {}", rm.roomId));
}

void RoomPopup::removeLoadingCircle() {
    if (loadingCircle) {
        loadingCircle->fadeAndRemove();
        loadingCircle = nullptr;
    }
}

void RoomPopup::reloadPlayerList() {
    // remove loading circle and existing list
    this->removeLoadingCircle();

    listLayer->m_list->removeFromParent();
    listLayer->m_list = Build<ListView>::create(CCArray::create(), PlayerListCell::CELL_HEIGHT, LIST_WIDTH, LIST_HEIGHT)
        .parent(listLayer)
        .collect();

    // send the request and show a new loading circle
    NetworkManager::get().send(RequestRoomPlayerListPacket::create());

    loadingCircle = LoadingCircle::create();
    // TODO 2.2 ??
    // loadingCircle->setParentLayer(listLayer);
    this->addChild(listLayer);
    loadingCircle->setPosition(-listLayer->getPosition());
    loadingCircle->show();
}

RoomPopup::~RoomPopup() {
    NetworkManager::get().removeListener<RoomPlayerListPacket>();
}

RoomPopup* RoomPopup::create() {
    auto ret = new RoomPopup;
    if (ret->init(POPUP_WIDTH, POPUP_HEIGHT)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}