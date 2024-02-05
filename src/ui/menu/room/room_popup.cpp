#include "room_popup.hpp"

#include "player_list_cell.hpp"
#include "room_join_popup.hpp"
#include <data/packets/all.hpp>
#include <net/network_manager.hpp>
#include <managers/error_queues.hpp>
#include <managers/profile_cache.hpp>
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
        auto& rm = RoomManager::get();
        bool changed = rm.roomId != packet->roomId;
        rm.setRoom(packet->roomId);
        this->onLoaded(changed || !roomBtnMenu);
    });

    nm.addListener<RoomJoinFailedPacket>([this](auto) {
        FLAlertLayer::create("Globed error", "Failed to join room: no room was found by the given ID.", "Ok")->show();
    });

    nm.addListener<RoomJoinedPacket>([this](auto) {
        this->reloadPlayerList(true);
    });

    nm.addListener<RoomCreatedPacket>([this](RoomCreatedPacket* packet) {
        auto ownData = ProfileCacheManager::get().getOwnData();
        auto* gjam = GJAccountManager::sharedState();

        this->playerList = {PlayerRoomPreviewAccountData(
            gjam->m_accountID,
            gjam->m_username,
            ownData.cube,
            ownData.color1,
            ownData.color2,
            ownData.glowColor,
            0
        )};

        RoomManager::get().setRoom(packet->roomId);
        this->onLoaded(true);
    });

    auto listview = ListView::create(CCArray::create(), PlayerListCell::CELL_HEIGHT, LIST_WIDTH, LIST_HEIGHT);
    listLayer = GJCommentListLayer::create(listview, "", {192, 114, 62, 255}, LIST_WIDTH, LIST_HEIGHT, false);

    float xpos = (m_mainLayer->getScaledContentSize().width - LIST_WIDTH) / 2;
    listLayer->setPosition({xpos, 85.f});
    m_mainLayer->addChild(listLayer);

    this->reloadPlayerList();

    CCScheduler::get()->scheduleSelector(schedule_selector(RoomPopup::selReloadPlayerList), this, 2.5f, false);

    return true;
}

void RoomPopup::selReloadPlayerList(float) {
    this->reloadPlayerList(true);
}

void RoomPopup::onLoaded(bool stateChanged) {
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

    if (stateChanged) {
        auto& rm = RoomManager::get();
        this->setTitle(rm.isInGlobal() ? "Global room" : fmt::format("Room {}", rm.roomId.load()));
        this->addButtons();
    }
}

void RoomPopup::addButtons() {
    // remove existing buttons
    if (roomBtnMenu) roomBtnMenu->removeFromParent();

    auto& rm = RoomManager::get();
    float popupCenter = CCDirector::get()->getWinSize().width / 2;

    if (rm.isInGlobal()) {
        Build<CCMenu>::create()
            .layout(
                RowLayout::create()
                ->setAxisAlignment(AxisAlignment::Center)
                ->setGap(5.0f)
            )
            .id("btn-menu"_spr)
            .pos(popupCenter, 55.f)
            .parent(m_mainLayer)
            .store(roomBtnMenu);

        Build<ButtonSprite>::create("Join room", "bigFont.fnt", "GJ_button_01.png", 0.8f)
            .intoMenuItem([this](auto) {
                if (!this->isLoading()) {
                    RoomJoinPopup::create()->show();
                }
            })
            .id("join-room-btn"_spr)
            .parent(roomBtnMenu);

        Build<ButtonSprite>::create("Create room", "bigFont.fnt", "GJ_button_01.png", 0.8f)
            .intoMenuItem([this](auto) {
                if (!this->isLoading()) {
                    NetworkManager::get().send(CreateRoomPacket::create());
                    this->reloadPlayerList(false);
                }
            })
            .id("create-room-btn"_spr)
            .parent(roomBtnMenu);

        roomBtnMenu->updateLayout();
    } else {
        Build<ButtonSprite>::create("Leave room", "bigFont.fnt", "GJ_button_01.png", 0.8f)
            .intoMenuItem([this](auto) {
                if (!this->isLoading()) {
                    NetworkManager::get().send(LeaveRoomPacket::create());
                    this->reloadPlayerList(false);
                }
            })
            .id("leave-room-btn"_spr)
            .intoNewParent(CCMenu::create())
            .id("leave-room-btn-menu"_spr)
            .pos(popupCenter, 55.f)
            .parent(m_mainLayer)
            .store(roomBtnMenu);
    }
}

void RoomPopup::removeLoadingCircle() {
    if (loadingCircle) {
        loadingCircle->fadeAndRemove();
        loadingCircle = nullptr;
    }
}

void RoomPopup::reloadPlayerList(bool sendPacket) {
    auto& nm = NetworkManager::get();
    if (!nm.established()) {
        this->onClose(this);
        return;
    }

    // remove loading circle and existing list
    this->removeLoadingCircle();

    // send the request
    if (sendPacket) {
        NetworkManager::get().send(RequestRoomPlayerListPacket::create());
    }

    // show the circle
    loadingCircle = LoadingCircle::create();
    loadingCircle->setParentLayer(listLayer);
    loadingCircle->setPosition(-listLayer->getPosition());
    loadingCircle->show();
}

bool RoomPopup::isLoading() {
    return loadingCircle != nullptr;
}

RoomPopup::~RoomPopup() {
    auto& nm = NetworkManager::get();

    nm.removeListener<RoomPlayerListPacket>();
    nm.removeListener<RoomJoinedPacket>();
    nm.removeListener<RoomJoinFailedPacket>();
    nm.removeListener<RoomCreatedPacket>();

    nm.suppressUnhandledFor<RoomPlayerListPacket>(util::time::secs(1));
    nm.suppressUnhandledFor<RoomJoinedPacket>(util::time::secs(1));
    nm.suppressUnhandledFor<RoomJoinFailedPacket>(util::time::secs(1));
    nm.suppressUnhandledFor<RoomCreatedPacket>(util::time::secs(1));
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