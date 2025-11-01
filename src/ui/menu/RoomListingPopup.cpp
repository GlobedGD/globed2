#include "RoomListingPopup.hpp"
#include "RoomListingCell.hpp"

#include <globed/core/SettingsManager.hpp>
#include <globed/core/PopupManager.hpp>
#include <globed/util/Random.hpp>
#include <ui/misc/InputPopup.hpp>
#include <ui/misc/LoadingPopup.hpp>
#include <core/net/NetworkManagerImpl.hpp>

#include <UIBuilder.hpp>
#include <asp/iter.hpp>

using namespace geode::prelude;

namespace globed {

const CCSize RoomListingPopup::POPUP_SIZE = {420.f, 280.f};
const CCSize RoomListingPopup::LIST_SIZE = {375.f, 200.f};

static std::vector<RoomListingInfo> makeFakeData();

bool RoomListingPopup::setup() {
    this->updateTitle(0);
    this->setID("room-listing"_spr);

    auto& nm = NetworkManagerImpl::get();
    if (!nm.isConnected()) {
        return false;
    }

    m_list = Build(cue::ListNode::create(LIST_SIZE))
        .anchorPoint(0.5f, 1.f)
        .pos(this->fromTop(35.f))
        .zOrder(2)
        .parent(m_mainLayer);

    m_list->setAutoUpdate(false);

    m_roomListListener = NetworkManagerImpl::get().listen<msg::RoomListMessage>([this](const auto& msg) {
        m_loading = false;
        m_totalRooms = msg.total;

        if (setting<bool>("core.dev.fake-data")) {
            auto rooms = makeFakeData();
            this->onPageLoaded(rooms, m_page);
        } else {
            this->onPageLoaded(msg.rooms, msg.page);
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
            this->onReload();
        })
        .pos(this->fromBottomRight(3.f, 3.f))
        .id("reload-btn")
        .parent(m_buttonMenu);

    Build<CCSprite>::createSpriteName("GJ_plusBtn_001.png")
        .scale(0.9f)
        .intoMenuItem([this](auto) {
            auto popup = InputPopup::create("chatFont.fnt");
            popup->setTitle("Join Room");
            popup->setPlaceholder("Room ID");
            popup->setCommonFilter(CommonFilter::Uint);
            popup->setMaxCharCount(7);
            popup->setCallback([this](auto outcome) {
                if (!outcome.cancelled) {
                    // parse the room ID
                    uint32_t id = geode::utils::numFromString<uint32_t>(outcome.text).unwrapOr(0);
                    this->doJoinRoom(id, false);
                }
            });
            popup->show();
        })
        .pos(this->fromBottomLeft(3.f, 3.f))
        .id("add-room-btn")
        .parent(m_buttonMenu);

    auto bottomMenu = Build<CCMenu>::create()
        .id("bottom-menu")
        .pos(this->fromBottom(22.f))
        .layout(
            RowLayout::create()
                ->setGap(8.f)
                ->setAutoScale(false)
        )
        .contentSize(150.f, 0.f)
        .parent(m_mainLayer)
        .collect();

    // mod actions button
    if (NetworkManagerImpl::get().isAuthorizedModerator()) {
        auto menu = Build<CCMenu>::create()
            .id("mod-actions-menu")
            .layout(
                RowLayout::create()
                    ->setGap(8.f)
                    ->setAutoScale(false)
            )
            .contentSize(60.f, 0.f)
            .parent(bottomMenu)
            .child(Build<CCLabelBMFont>::create("Mod", "bigFont.fnt").scale(0.4f))
            .child(
                Build(CCMenuItemExt::createTogglerWithStandardSprites(0.6f, [this](auto btn) {
                    bool enabled = !btn->isToggled();
                    this->toggleModActions(enabled);
                }))
            )
            .zOrder(20)
            .updateLayout()
            .collect();

        cue::attachBackground(menu);
    }

    // page arrow buttons
    Build<CCSprite>::createSpriteName("GJ_arrow_03_001.png")
        .scale(0.7f)
        .intoMenuItem([this](auto) {
            this->switchPage(-1);
        })
        .zOrder(10)
        .id("btn-prev-page")
        .parent(bottomMenu);

    Build<CCSprite>::createSpriteName("GJ_arrow_03_001.png")
        .flipX(true)
        .scale(0.7f)
        .intoMenuItem([this](auto) {
            this->switchPage(1);
        })
        .zOrder(30)
        .id("btn-prev-page")
        .parent(bottomMenu);

    bottomMenu->updateLayout();

    float btnSize = 32.f;

    // search and clear search buttons
    m_searchBtn = Build<CCSprite>::create("search01.png"_spr)
        .with([&](auto spr) { cue::rescaleToMatch(spr, btnSize); })
        .intoMenuItem([this] {
            this->promptFilter();
        })
        .zOrder(8)
        .scaleMult(1.1f)
        .id("search-btn")
        .pos(this->fromBottomLeft(40.f, 23.f))
        .parent(m_buttonMenu);

    m_clearSearchBtn = Build<CCSprite>::create("search02.png"_spr)
        .with([&](auto spr) { cue::rescaleToMatch(spr, btnSize); })
        .intoMenuItem([this] {
            this->setFilter("");
        })
        .zOrder(8)
        .scaleMult(1.1f)
        .id("clear-search-btn")
        .pos(this->fromBottomLeft(40.f, 23.f))
        .parent(m_buttonMenu);

    this->onReload();

    // fuck if i know
    cocos::handleTouchPriority(this, true);

    return true;
}

void RoomListingPopup::promptFilter() {
    auto popup = InputPopup::create("bigFont.fnt");
    popup->setTitle("Filter Rooms");
    popup->setPlaceholder("Name");
    popup->setMaxCharCount(24);
    popup->setCommonFilter(CommonFilter::Any);
    popup->setWidth(240.f);
    popup->setCallback([this](auto outcome) {
        if (outcome.cancelled) return;

        this->setFilter(outcome.text);
    });
    popup->show();
}

void RoomListingPopup::setFilter(std::string_view filter) {
    m_filter = filter;
    this->onReload();
}

void RoomListingPopup::onReload() {
    m_loadedPages = 0;
    m_page = 0;
    m_maxPages = 0;
    m_totalRooms = 0;
    m_roomCount = 0;
    m_allRooms.clear();
    m_list->clear();
    this->requestRooms();
}

void RoomListingPopup::requestRooms() {
    if (m_loading) return;

    log::debug("Requesting page {}", m_loadedPages);

    m_loading = true;
    NetworkManagerImpl::get().sendRequestRoomList(m_filter, m_loadedPages);

    m_clearSearchBtn->setVisible(!m_filter.empty());
    m_searchBtn->setVisible(m_filter.empty());
}

void RoomListingPopup::updateTitle(size_t roomCount) {
    m_roomCount = roomCount;
    this->setTitle(fmt::format("Public Rooms ({} Rooms)", roomCount), "goldFont.fnt", 0.7f, 17.f);
}

void RoomListingPopup::populateList() {
    log::debug("Rendering page {}", m_page);
    GLOBED_ASSERT(m_page < m_allRooms.size());

    m_list->clear();

    for (auto room : asp::iter::from(m_allRooms[m_page])) {
        m_list->addCell(RoomListingCell::create(room.get(), this));
    }

    this->updateTitle(m_totalRooms ?: m_list->size());

    m_list->sortAs<RoomListingCell>([](RoomListingCell* a, RoomListingCell* b) {
        return a->getPlayerCount() > b->getPlayerCount();
    });

    m_list->updateLayout();

    cocos::handleTouchPriority(this, true);

    this->toggleModActions(m_modActionsOn);
}

void RoomListingPopup::onPageLoaded(const std::vector<RoomListingInfo>& rooms, uint32_t page) {
    if (rooms.empty()) {
        // no rooms on this page, means the previous page was the last one
        m_maxPages = m_loadedPages;
        m_page = std::clamp<uint32_t>(m_page, 0, m_maxPages - 1);
        return;
    }

    m_loadedPages = std::max(page + 1, m_loadedPages);
    m_page = std::clamp<uint32_t>(m_page, 0, m_loadedPages - 1);

    if (page >= m_allRooms.size()) {
        m_allRooms.resize(page + 1);
    }

    m_allRooms[page] = rooms;

    log::debug("Rooms on page {}: {}; total: {}", page, rooms.size(), m_totalRooms);

    this->populateList();
}

void RoomListingPopup::switchPage(int delta) {
    if (m_loadedPages == 0 || m_loading || (m_page == 0 && delta < 0)) return;

    if (m_maxPages != 0 && (m_page + delta) >= m_maxPages) {
        return;
    }

    m_page += delta;
    bool isPageLoaded = m_page < m_loadedPages && m_page < m_allRooms.size() && !m_allRooms[m_page].empty();

    if (isPageLoaded) {
        this->populateList();
    } else {
        this->requestRooms();
    }
}

void RoomListingPopup::toggleModActions(bool enabled) {
    m_modActionsOn = enabled;

    for (auto cell : m_list->iter<RoomListingCell>()) {
        cell->toggleModActions(enabled);
    }
}

void RoomListingPopup::doJoinRoom(uint32_t roomId, bool hasPassword) {
    if (roomId == 0) {
        globed::alert("Error", "Invalid room ID");
        return;
    }

    auto& nm = NetworkManagerImpl::get();

    if (hasPassword) {
        // prompt for password
        auto popup = InputPopup::create("chatFont.fnt");
        popup->setTitle("Room Password");
        popup->setPlaceholder("Room Password");
        popup->setCommonFilter(CommonFilter::Uint);
        popup->setMaxCharCount(16);
        popup->setCallback([this, roomId](auto outcome) {
            if (!outcome.cancelled) {
                // parse the password
                uint64_t pw = geode::utils::numFromString<uint64_t>(outcome.text).unwrapOr(0);
                this->actuallyJoin(roomId, pw);
            }
        });
        popup->show();
    } else {
        this->actuallyJoin(roomId, 0);
    }
}

void RoomListingPopup::actuallyJoin(uint32_t roomId, uint64_t passcode) {
    m_joinedRoomId = roomId;
    this->waitForResponse();
    NetworkManagerImpl::get().sendJoinRoom(roomId, passcode);
}

void RoomListingPopup::waitForResponse() {
    // wait for either a room state mesage or a room join failed message

    m_loadingPopup = LoadingPopup::create();
    m_loadingPopup->setTitle("Joining Room...");
    m_loadingPopup->setClosable(true);
    m_loadingPopup->show();

    m_successListener = NetworkManagerImpl::get().listen<msg::RoomStateMessage>([this](const auto& msg) {
        // small sanity check to make sure it is actually the response we need
        if (msg.roomId == m_joinedRoomId) {
            this->stopWaiting(std::nullopt);
        }

        return ListenerResult::Continue;
    });
    m_successListener.value()->setPriority(-100);

    m_failListener = NetworkManagerImpl::get().listen<msg::RoomJoinFailedMessage>([this](const auto& msg) {
        using enum msg::RoomJoinFailedReason;
        std::string reason;

        switch (msg.reason) {
            case NotFound: reason = "Room not found"; break;
            case InvalidPasscode: {
                this->stopWaiting(std::nullopt);
                this->doJoinRoom(m_joinedRoomId, true);
            } break;
            case Full: reason = "Room is full"; break;
            case Banned: reason = "You are banned from this room"; break;
            default: reason = "Unknown reason"; break;
        }

        this->stopWaiting(reason);

        return ListenerResult::Stop;
    });
    m_failListener.value()->setPriority(-1);
}

void RoomListingPopup::stopWaiting(std::optional<std::string> failReason) {
    log::debug("Stop waiting!");
    if (m_loadingPopup) {
        m_loadingPopup->forceClose();
        m_loadingPopup = nullptr;
    }

    m_failListener.reset();
    m_successListener.reset();

    if (failReason) {
        globed::alertFormat("Error", "Failed to join room: <cy>{}</c>", *failReason);
    } else {
        this->onClose(nullptr);
    }
}

void RoomListingPopup::doRemoveCell(RoomListingCell* cell) {
    auto idx = m_list->indexForCell(static_cast<cue::ListCell*>(cell->getParent()));
    m_list->removeCell(idx);
    this->updateTitle(m_roomCount - 1);
}

static std::vector<RoomListingInfo> makeFakeData() {
    std::vector<RoomListingInfo> rooms;

    size_t count = rng()->random<size_t>(500, 1000);
    for (size_t i = 0; i < count; i++) {
        rooms.push_back(RoomListingInfo {
            .roomId = rng()->random<uint32_t>(100000, 999999),
            .roomName = fmt::format("Room {}", i + 1),
            .roomOwner = RoomPlayer {
                MinimalRoomPlayer {
                    .accountData = PlayerAccountData {
                        .accountId = rng()->random<int>(),
                        .userId = rng()->random<int>(),
                        .username = fmt::format("Player {}", rng()->random<int>(1, 1000000))
                    },
                    .cube = rng()->random<int16_t>(1, 484),
                    .color1 = rng()->random<uint16_t>(1, 106),
                    .color2 = rng()->random<uint16_t>(1, 106),
                    .glowColor = NO_GLOW,
                },
                SessionId{},
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
