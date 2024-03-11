#include "room_popup.hpp"

#include "player_list_cell.hpp"
#include "room_join_popup.hpp"
#include <data/packets/all.hpp>
#include <net/network_manager.hpp>
#include <managers/error_queues.hpp>
#include <managers/profile_cache.hpp>
#include <managers/friend_list.hpp>
#include <managers/room.hpp>
#include <ui/general/ask_input_popup.hpp>
#include <util/ui.hpp>
#include <util/misc.hpp>
#include <util/format.hpp>

using namespace geode::prelude;

bool RoomPopup::setup() {
    auto& nm = NetworkManager::get();
    if (!nm.established()) {
        return false;
    }

    FriendListManager::get().maybeLoad();

    auto& rm = RoomManager::get();

    nm.addListener<RoomPlayerListPacket>([this](std::shared_ptr<RoomPlayerListPacket> packet) {
        this->isWaiting = false;
        this->playerList = packet->data;
        this->applyFilter("");
        this->sortPlayerList();
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

    nm.addListener<RoomCreatedPacket>([this](std::shared_ptr<RoomCreatedPacket> packet) {
        auto ownData = ProfileCacheManager::get().getOwnData();
        auto ownSpecialData = ProfileCacheManager::get().getOwnSpecialData();

        auto* gjam = GJAccountManager::sharedState();

        this->playerList = {PlayerRoomPreviewAccountData(
            gjam->m_accountID,
            GameManager::get()->m_playerUserID.value(),
            gjam->m_username,
            ownData.cube,
            ownData.color1,
            ownData.color2,
            ownData.glowColor,
            0,
            ownSpecialData
        )};
        this->applyFilter("");

        RoomManager::get().setRoom(packet->roomId);
        this->onLoaded(true);
    });

    auto listview = ListView::create(CCArray::create(), PlayerListCell::CELL_HEIGHT, LIST_WIDTH, LIST_HEIGHT);
    listLayer = GJCommentListLayer::create(listview, "", {192, 114, 62, 255}, LIST_WIDTH, LIST_HEIGHT, false);

    float xpos = (m_mainLayer->getScaledContentSize().width - LIST_WIDTH) / 2;
    listLayer->setPosition({xpos, 85.f});
    m_mainLayer->addChild(listLayer);

    this->reloadPlayerList();

    Build<CCSprite>::createSpriteName("GJ_updateBtn_001.png")
        .scale(0.9f)
        .intoMenuItem([this](auto) {
            this->reloadPlayerList(true);
        })
        .pos(m_size.width / 2.f - 3.f, -m_size.height / 2.f + 3.f)
        .id("reload-btn"_spr)
        .intoNewParent(CCMenu::create())
        .parent(m_mainLayer);

    CCMenuItemSpriteExtra* filterBtn = nullptr;

    Build<CCSprite>::createSpriteName("gj_findBtn_001.png")
        .scale(0.9f)
        .intoMenuItem([this](auto) {
            AskInputPopup::create("Search Player", [this](const std::string_view input) {
                this->applyFilter(input);
                this->sortPlayerList();
                this->onLoaded(true);
            }, 16, "Username", util::misc::STRING_ALPHANUMERIC, 3.f)->show();
        })
        .pos(m_size.width / 2.f - 21.f, m_size.height / 2.f - 22.f)
        .id("search-btn"_spr)
        .store(filterBtn)
        .intoNewParent(CCMenu::create())
        .parent(m_mainLayer);
    filterBtn->m_scaleMultiplier = 1.1f;

    // clear search button
    Build<CCSprite>::createSpriteName("gj_findBtnOff_001.png")
        .scale(0.9f)
        .intoMenuItem([this](auto) {
            this->applyFilter("");
            this->sortPlayerList();
            this->onLoaded(true);
        })
        .visible(false)
        .pos(m_size.width / 2.f - 21.f, m_size.height / 2.f - 40.f - filterBtn->getScaledContentSize().height / 2)
        .id("search-clear-btn"_spr)
        .store(clearSearchButton)
        .intoNewParent(CCMenu::create())
        .parent(m_mainLayer);
    clearSearchButton->m_scaleMultiplier = 1.1f;

    return true;
}

void RoomPopup::onLoaded(bool stateChanged) {
    this->removeLoadingCircle();

    auto cells = CCArray::create();

    for (const auto& pdata : filteredPlayerList) {
        auto* cell = PlayerListCell::create(pdata);
        cells->addObject(cell);
    }

    // preserve scroll position
    float scrollPos = util::ui::getScrollPos(listLayer->m_list);
    int previousCellCount = listLayer->m_list->m_entries->count();

    listLayer->m_list->removeFromParent();
    listLayer->m_list = Build<ListView>::create(cells, PlayerListCell::CELL_HEIGHT, LIST_WIDTH, LIST_HEIGHT)
        .parent(listLayer)
        .collect();

    if (previousCellCount != 0 && !stateChanged) {
        util::ui::setScrollPos(listLayer->m_list, scrollPos);
    }

    if (stateChanged) {
        auto& rm = RoomManager::get();
        this->setRoomTitle(rm.roomId.load());
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

        auto* joinRoomButton = Build<ButtonSprite>::create("Join room", "bigFont.fnt", "GJ_button_01.png", 0.8f)
            .intoMenuItem([this](auto) {
                if (!this->isLoading()) {
                    RoomJoinPopup::create()->show();
                }
            })
            .id("join-room-btn"_spr)
            .parent(roomBtnMenu)
            .collect();
        joinRoomButton->m_scaleMultiplier = 1.05f;

        auto* createRoomButton = Build<ButtonSprite>::create("Create room", "bigFont.fnt", "GJ_button_01.png", 0.8f)
            .intoMenuItem([this](auto) {
                if (!this->isLoading()) {
                    NetworkManager::get().send(CreateRoomPacket::create());
                    this->reloadPlayerList(false);
                }
            })
            .id("create-room-btn"_spr)
            .parent(roomBtnMenu)
            .collect();
        createRoomButton->m_scaleMultiplier = 1.05f;

        roomBtnMenu->updateLayout();
    } else {
        Build<CCMenu>::create()
            .id("leave-room-btn-menu"_spr)
            .pos(popupCenter, 55.f)
            .parent(m_mainLayer)
            .store(roomBtnMenu);

        auto* leaveRoomButton = Build<ButtonSprite>::create("Leave room", "bigFont.fnt", "GJ_button_01.png", 0.8f)
            .intoMenuItem([this](auto) {
                if (!this->isLoading()) {
                    NetworkManager::get().send(LeaveRoomPacket::create());
                    this->reloadPlayerList(false);
                }
            })
            .id("leave-room-btn"_spr)
            .parent(roomBtnMenu)
            .collect();
        leaveRoomButton->m_scaleMultiplier = 1.1f;
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

    // remove loading circle
    this->removeLoadingCircle();

    // send the request
    if (sendPacket) {
        if (!isWaiting) {
            NetworkManager::get().send(RequestRoomPlayerListPacket::create());
            isWaiting = true;
        }
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

void RoomPopup::sortPlayerList() {
    auto& flm = FriendListManager::get();

    // filter out the weird people (old game server used to send unauthenticated people too)
    filteredPlayerList.erase(std::remove_if(filteredPlayerList.begin(), filteredPlayerList.end(), [](const auto& player) {
        return player.accountId == 0;
    }), filteredPlayerList.end());

    // show friends before everyone else, and sort everyone alphabetically by the name
    std::sort(filteredPlayerList.begin(), filteredPlayerList.end(), [&flm](const auto& p1, const auto& p2) -> bool {
        bool isFriend1 = flm.isFriend(p1.accountId);
        bool isFriend2 = flm.isFriend(p2.accountId);

        if (isFriend1 != isFriend2) {
            return isFriend1;
        } else {
            // convert both names to lowercase
            std::string name1 = p1.name, name2 = p2.name;
            std::transform(name1.begin(), name1.end(), name1.begin(), ::tolower);
            std::transform(name2.begin(), name2.end(), name2.begin(), ::tolower);

            // sort alphabetically
            return name1 < name2;
        }
    });
}

void RoomPopup::applyFilter(const std::string_view input) {
    filteredPlayerList.clear();

    if (input.empty()) {
        for (const auto& item : playerList) {
            filteredPlayerList.push_back(item);
        }

        clearSearchButton->setVisible(false);
        return;
    }

    auto filt = util::format::toLowercase(input);

    for (const auto& item : playerList) {
        auto name = util::format::toLowercase(item.name);
        if (name.find(filt) != std::string::npos) {
            filteredPlayerList.push_back(item);
        }
    }

    clearSearchButton->setVisible(true);
}

void RoomPopup::setRoomTitle(uint32_t id) {
    if (roomIdButton) roomIdButton->removeFromParent();

    auto title = id == 0 ? "Global room" : fmt::format("Room {}", id);
    auto layout = util::ui::getPopupLayout(m_size);

    CCNode* elem = Build<CCLabelBMFont>::create(title.c_str(), "goldFont.fnt")
        .scale(0.7f)
        .collect();

    if (id != 0) {
        auto* button = CCMenuItemSpriteExtra::create(elem, this, menu_selector(RoomPopup::onCopyRoomId));
        button->addChild(elem);

        auto* menu = CCMenu::create();
        menu->addChild(button);

        elem = menu;
    }

    elem->setPosition(layout.centerTop - CCPoint{0.f, 17.f});
    m_mainLayer->addChild(elem);

    roomIdButton = elem;
}

void RoomPopup::onCopyRoomId(cocos2d::CCObject*) {
    auto id = RoomManager::get().roomId.load();

    utils::clipboard::write(std::to_string(id));

    Notification::create("Copied to clipboard", NotificationIcon::Success)->show();
}

RoomPopup::~RoomPopup() {
    auto& nm = NetworkManager::get();

    nm.removeListener<RoomPlayerListPacket>(util::time::seconds(2));
    nm.removeListener<RoomJoinedPacket>(util::time::seconds(2));
    nm.removeListener<RoomJoinFailedPacket>(util::time::seconds(2));
    nm.removeListener<RoomCreatedPacket>(util::time::seconds(2));
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