#include "RoomListingPopup.hpp"
#include "RoomListingCell.hpp"

#include <globed/core/SettingsManager.hpp>
#include <globed/util/Random.hpp>
#include <core/net/NetworkManagerImpl.hpp>
#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

CCSize RoomListingPopup::POPUP_SIZE = {420.f, 240.f};
CCSize RoomListingPopup::LIST_SIZE = {375.f, 160.f};

static std::vector<RoomListingInfo> makeFakeData();

bool RoomListingPopup::setup() {
    this->updateTitle(0);
    this->setID("room-listing"_spr);

    auto& nm = NetworkManagerImpl::get();
    if (!nm.isConnected()) {
        return false;
    }

    m_list = Build(cue::ListNode::create(LIST_SIZE, ccColor4B{0x33, 0x44, 0x99, 255}, cue::ListBorderStyle::Comments))
        .anchorPoint(0.5f, 1.f)
        .pos(this->fromTop(40.f))
        .zOrder(2)
        .parent(m_mainLayer);

    m_roomListListener = NetworkManagerImpl::get().listen<msg::RoomListMessage>([this](const auto& msg) {
        if (setting<bool>("core.dev.fake-data")) {
            log::debug("Using fake data for room listing");
            auto rooms = makeFakeData();
            this->populateList(rooms);
        } else {
            this->populateList(msg.rooms);
        }

        return ListenerResult::Continue;
    });

    m_background = Build<CCScale9Sprite>::create("square02_small.png")
        .id("background")
        .contentSize(LIST_SIZE)
        .opacity(75)
        .anchorPoint(0.5f, 1.f)
        .pos(m_list->getPosition())
        .parent(m_mainLayer)
        .zOrder(1);

    Build<CCSprite>::createSpriteName("GJ_updateBtn_001.png")
        .scale(0.9f)
        .intoMenuItem([this](auto) {
            this->onReload(nullptr);
        })
        .pos(this->fromBottomRight(3.f, 3.f))
        .id("reload-btn")
        .parent(m_buttonMenu);

    Build<CCSprite>::createSpriteName("GJ_plusBtn_001.png")
        .scale(0.9f)
        .intoMenuItem([this](auto) {
            // TODO: show a popup to join a room
            // auto popup = InputPopup::create("chatFont.fnt");
            // popup->setPlaceholder("Room ID");
            // popup->setCommonFilter(CommonFilter::Uint);
            // popup->setMaxCharCount(7);
            // popup->setCallback([](auto outcome) {
            //     if (!outcome.cancelled) {
            //         // parse the room ID
            //         uint32_t id = geode::utils::numFromString<uint32_t>(outcome.text).unwrapOr(0);
            //         if (id == 0) {
            //             // TODO: popupmanager
            //             FLAlertLayer::create("Error", "Invalid room ID", "Ok")->show();
            //             return;
            //         }

            //         NetworkManagerImpl::get().sendJoinRoom(id, 0); // TODO: passcode
            //     }
            // });
            // popup->show();
        })
        .pos(this->fromBottomLeft(3.f, 3.f))
        .id("add-room-btn")
        .parent(m_buttonMenu);

    // TODO: if mod, show a button to toggle mod actions

    NetworkManagerImpl::get().sendRequestRoomList();

    return true;
}

void RoomListingPopup::onReload(CCObject*) {
    NetworkManagerImpl::get().sendRequestRoomList();
}

void RoomListingPopup::updateTitle(size_t roomCount) {
    m_roomCount = roomCount;
    this->setTitle(fmt::format("Public Rooms ({} Rooms)", roomCount));
}

void RoomListingPopup::populateList(const std::vector<RoomListingInfo>& rooms) {
    this->updateTitle(rooms.size());

    m_list->setAutoUpdate(false);
    m_list->clear();

    for (auto& room : rooms) {
        m_list->addCell(RoomListingCell::create(room, this));
    }

    m_list->sortAs<RoomListingCell>([](RoomListingCell* a, RoomListingCell* b) {
        return a->getPlayerCount() > b->getPlayerCount();
    });

    m_list->setAutoUpdate(true);
    m_list->updateLayout();
    m_list->scrollToTop();

    this->toggleModActions(m_modActionsOn);
}

void RoomListingPopup::toggleModActions(bool enabled) {
    m_modActionsOn = enabled;

    for (auto cell : m_list->iter<RoomListingCell>()) {
        cell->toggleModActions(enabled);
    }
}

static std::vector<RoomListingInfo> makeFakeData() {
    std::vector<RoomListingInfo> rooms;

    size_t count = rng()->random<size_t>(50, 100);
    for (size_t i = 0; i < count; i++) {
        rooms.push_back(RoomListingInfo {
            .roomId = rng()->random<uint32_t>(100000, 999999),
            .roomName = fmt::format("Room {}", i + 1),
            .roomOwner = RoomPlayer {
                .accountData = PlayerAccountData {
                    .accountId = rng()->random<int>(),
                    .userId = rng()->random<int>(),
                    .username = fmt::format("Player {}", rng()->random<int>())
                },
                .cube = rng()->random<int16_t>(1, 484),
                .color1 = rng()->random<uint16_t>(1, 106),
                .color2 = rng()->random<uint16_t>(1, 106),
                .glowColor = NO_GLOW,
                .session = SessionId{},
            },
            .playerCount = rng()->random<uint32_t>(1, 1000),
            .hasPassword = rng()->randomRatio(0.3),
            .settings = RoomSettings {
                .serverId = 0,
                .playerLimit = rng()->randomRatio(0.1) ? rng()->random<uint16_t>(1, 2000) : (uint16_t)0,
                .fasterReset = rng()->random<bool>(),
                .hidden = rng()->randomRatio(0.1),
                .privateInvites = rng()->randomRatio(0.2),
                .isFollower = rng()->randomRatio(0.2),
                .levelIntegrity = rng()->randomRatio(0.1),
                .collision = rng()->randomRatio(0.2),
                .twoPlayerMode = rng()->randomRatio(0.2),
                .deathlink = rng()->randomRatio(0.2),
            },
        });
    }

    return rooms;
}

}
