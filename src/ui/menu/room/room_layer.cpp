#include "room_layer.hpp"

#include "player_list_cell.hpp"
#include "room_join_popup.hpp"
#include "room_settings_popup.hpp"
#include "room_listing_popup.hpp"
#include "invite_popup.hpp"
#include "create_room_popup.hpp"
#include <data/packets/all.hpp>
#include <net/manager.hpp>
#include <managers/error_queues.hpp>
#include <managers/profile_cache.hpp>
#include <managers/friend_list.hpp>
#include <managers/settings.hpp>
#include <managers/room.hpp>
#include <ui/general/ask_input_popup.hpp>
#include <util/ui.hpp>
#include <util/cocos.hpp>
#include <util/misc.hpp>
#include <util/format.hpp>
#include <util/math.hpp>

using namespace geode::prelude;

bool RoomLayer::init() {
    if (!CCLayer::init()) return false;

    auto winSize = CCDirector::get()->getWinSize();

    const float popupWidth = util::ui::capPopupWidth(420.f);
    const float popupHeight = 280.f;

    listWidth = popupWidth * 0.80f;
    listHeight = 180.f;

    popupSize = CCSize{popupWidth, popupHeight};

    auto bg = CCScale9Sprite::create("GJ_square02.png", {0, 0, 80, 80});
    bg->setContentSize(popupSize);
    bg->setPosition(winSize.width / 2, winSize.height / 2);
    this->addChild(bg);

    auto& nm = NetworkManager::get();
    if (!nm.established()) {
        return false;
    }

    FriendListManager::get().maybeLoad();

    auto& rm = RoomManager::get();

    nm.addListener<RoomPlayerListPacket>(this, [this](std::shared_ptr<RoomPlayerListPacket> packet) {
        this->isWaiting = false;
        this->playerList = packet->players;
        this->applyFilter("");
        this->sortPlayerList();
        auto& rm = RoomManager::get();
        bool changed = rm.getId() != packet->info.id;
        rm.setInfo(packet->info);
        this->onLoaded(changed || !roomBtnMenu);
    });

    nm.addListener<RoomJoinedPacket>(this, [this](auto) {
        this->reloadPlayerList(true);
    });

    nm.addListener<RoomCreatedPacket>(this, [this](std::shared_ptr<RoomCreatedPacket> packet) {
        auto ownData = ProfileCacheManager::get().getOwnData();
        auto ownSpecialData = ProfileCacheManager::get().getOwnSpecialData();

        auto* gjam = GJAccountManager::sharedState();

        this->playerList = {PlayerRoomPreviewAccountData(
            gjam->m_accountID,
            GameManager::get()->m_playerUserID.value(),
            gjam->m_username,
            PlayerIconDataSimple(ownData),
            0,
            ownSpecialData
        )};
        this->applyFilter("");

        RoomManager::get().setInfo(packet->info);
        this->onLoaded(true);
    });

    nm.addListener<RoomCreateFailedPacket>(this, [this](std::shared_ptr<RoomCreateFailedPacket> packet) {
        ErrorQueues::get().error(fmt::format("Failed to create room: <cy>{}</c>", packet->reason));
        this->onLoaded(true);
    });

    nm.addListener<RoomInfoPacket>(this, [this](std::shared_ptr<RoomInfoPacket> packet) {
        ErrorQueues::get().success("Room configuration updated");

        RoomManager::get().setInfo(packet->info);
        // idk maybe refresh or something something idk

        this->recreateInviteButton();
    });

    auto rlayout = util::ui::getPopupLayout(popupSize);
    Build<PlayerList>::create(listWidth, listHeight, util::ui::BG_COLOR_DARK_BLUE, PlayerListCell::CELL_HEIGHT, GlobedListBorderType::GJCommentListLayerBlue)
        .ignoreAnchorPointForPos(false)
        .anchorPoint(0.5f, 1.f)
        .pos(rlayout.center.width, rlayout.top - 40.f)
        .parent(this)
        .store(listLayer)
        ;

    listLayer->setCellColors(util::ui::BG_COLOR_DARKER_BLUE, util::ui::BG_COLOR_DARK_BLUE);

    const float sidePadding = (rlayout.popupSize.width - listWidth) / 2.f;

    Build<CCMenu>::create()
        .layout(ColumnLayout::create()->setGap(1.f)->setAxisAlignment(AxisAlignment::End)->setAxisReverse(true))
        .scale(0.875f)
        // this is funny
        .pos(rlayout.right - util::math::min(6.f, (sidePadding - 28.f)), rlayout.top - 6.f)
        .anchorPoint(1.f, 1.f)
        .contentSize(30.f, popupHeight)
        .parent(this)
        .id("top-right-buttons"_spr)
        .store(buttonMenu);

    // this is grand
    float buttonSize = util::math::min(32.5f, sidePadding - 5.f);
    this->targetButtonSize = CCSize{buttonSize, buttonSize};

    CCMenuItemSpriteExtra *filterBtn;

    // status button
    auto* invisSprite = CCSprite::createWithSpriteFrameName("status-invisible.png"_spr);
    auto* visSprite = CCSprite::createWithSpriteFrameName("status-visible.png"_spr);
    util::ui::rescaleToMatch(invisSprite, targetButtonSize);
    util::ui::rescaleToMatch(visSprite, targetButtonSize);

    Build<CCMenuItemToggler>(CCMenuItemToggler::create(invisSprite, visSprite, this, menu_selector(RoomLayer::onChangeStatus)))
        .id("status-btn"_spr)
        .parent(buttonMenu)
        .store(statusButton);

    statusButton->m_offButton->m_scaleMultiplier = 1.1f;
    statusButton->m_onButton->m_scaleMultiplier = 1.1f;

    statusButton->toggle(!GlobedSettings::get().globed.isInvisible);

    // search button
    Build<CCSprite>::createSpriteName("gj_findBtn_001.png")
        .with([&](auto* btn) {
            util::ui::rescaleToMatch(btn, targetButtonSize);
        })
        .intoMenuItem([this](auto) {
            AskInputPopup::create("Search Player", [this](const std::string_view input) {
                this->applyFilter(input);
                this->sortPlayerList();
                this->onLoaded(true);
            }, 16, "Username", util::misc::STRING_ALPHANUMERIC, 3.f)->show();
        })
        .scaleMult(1.1f)
        .id("search-btn"_spr)
        .parent(buttonMenu)
        .store(filterBtn);

    // clear search button
    Build<CCSprite>::createSpriteName("gj_findBtnOff_001.png")
        .with([&](auto* btn) {
            util::ui::rescaleToMatch(btn, targetButtonSize);
        })
        .intoMenuItem([this](auto) {
            this->applyFilter("");
            this->sortPlayerList();
            this->onLoaded(true);
        })
        .scaleMult(1.1f)
        .id("search-clear-btn"_spr)
        .store(clearSearchButton);

    // invite button
    Build<CCSprite>::createSpriteName("icon-invite-menu.png"_spr)
        .with([this](CCSprite* spr) {
            util::ui::rescaleToMatch(spr, targetButtonSize);
        })
        .intoMenuItem([this](auto) {
            InvitePopup::create()->show();
        })
        .scaleMult(1.15f)
        .zOrder(9)
        .id("invite-btn"_spr)
        .parent(buttonMenu)
        .store(inviteButton);

    // settings button
    Build<CCSprite>::createSpriteName("accountBtn_settings_001.png")
        .scale(0.7f)
        .intoMenuItem([this](auto) {
            RoomSettingsPopup::create()->show();
        })
        .scaleMult(1.1f)
        .id("settings-button"_spr)
        .pos(22.f, 23.f)
        .visible(false)
        .store(settingsButton)
        .intoNewParent(CCMenu::create())
        .pos(rlayout.bottomLeft)
        .parent(this);

    // refresh button
    Build<CCSprite>::createSpriteName("icon-refresh-square.png"_spr)
        .with([&](auto* btn) {
            util::ui::rescaleToMatch(btn, targetButtonSize);
        })
        .intoMenuItem([this](auto) {
            this->reloadPlayerList(true);
        })
        .scaleMult(1.1f)
        .zOrder(999) // force below everything
        .id("reload-btn"_spr)
        .store(refreshButton)
        .parent(buttonMenu);

    buttonMenu->updateLayout();

    this->recreateInviteButton();
    this->scheduleUpdate();

    this->reloadPlayerList();

    return true;
}

void RoomLayer::onChangeStatus(CCObject*) {
    GlobedSettings& settings = GlobedSettings::get();

    if (!settings.flags.seenStatusNotice) {
        settings.flags.seenStatusNotice = true;

        FLAlertLayer::create(
            "Invisibility",
            "This button toggles whether you want to be visible on the global player list or not. You will still be visible to other players on the same level as you.",
            "OK"
        )->show();
    }

    bool invisible = statusButton->isOn();
    NetworkManager::get().send(UpdatePlayerStatusPacket::create(invisible));
    settings.globed.isInvisible = invisible;
}

void RoomLayer::update(float) {
    settingsButton->setVisible(RoomManager::get().isInRoom());
}

void RoomLayer::onLoaded(bool stateChanged) {
    this->removeLoadingCircle();

    auto cells = CCArray::create();

    for (const auto& pdata : filteredPlayerList) {
        auto* cell = PlayerListCell::create(pdata, listWidth, false);
        cells->addObject(cell);
    }

    int previousCellCount = listLayer->cellCount();

    listLayer->swapCells(cells);

    CCPoint rect[4] = {
        CCPoint(0, 0),
        CCPoint(0, PlayerListCell::CELL_HEIGHT - 1.f),
        CCPoint(listWidth, PlayerListCell::CELL_HEIGHT - 1.f),
        CCPoint(listWidth, 0)
    };

    if (previousCellCount == 0) {
        listLayer->scrollToTop();
    }

    auto& rm = RoomManager::get();
    if (stateChanged) {
        this->setRoomTitle(rm.getInfo().name, rm.getId());
        this->addButtons();
    }

    buttonMenu->updateLayout();

    this->recreateInviteButton();
}

void RoomLayer::addButtons() {
    // remove existing buttons
    if (roomBtnMenu) roomBtnMenu->removeFromParent();

    auto& rm = RoomManager::get();

    auto rlayout = util::ui::getPopupLayout(popupSize);

    if (rm.isInGlobal()) {
        Build<CCMenu>::create()
            .layout(
                RowLayout::create()
                ->setAxisAlignment(AxisAlignment::Center)
                ->setGap(5.0f)
            )
            .contentSize(listWidth, 0.f)
            .id("btn-menu"_spr)
            .pos(rlayout.center.width, rlayout.bottom + 30.f)
            .parent(this)
            .store(roomBtnMenu);

        auto* joinRoomButton = Build<ButtonSprite>::create("Join room", "bigFont.fnt", "GJ_button_01.png", 0.8f)
            .intoMenuItem([this](auto) {
                if (!this->isLoading()) {
                    //RoomJoinPopup::create()->show();
                    RoomListingPopup::create()->show();
                }
            })
            .scaleMult(1.05f)
            .id("join-room-btn"_spr)
            .parent(roomBtnMenu)
            .collect();

        auto* createRoomButton = Build<ButtonSprite>::create("Create room", "bigFont.fnt", "GJ_button_01.png", 0.8f)
            .intoMenuItem([this](auto) {
                if (!this->isLoading()) {
                    // TODO: do more with this
                    CreateRoomPopup::create(this)->show();
                }
            })
            .scaleMult(1.05f)
            .id("create-room-btn"_spr)
            .parent(roomBtnMenu)
            .collect();

        roomBtnMenu->updateLayout();
    } else {
        Build<CCMenu>::create()
            .id("leave-room-btn-menu"_spr)
            .pos(rlayout.center.width, rlayout.bottom + 30.f)
            .parent(this)
            .store(roomBtnMenu);

        auto* leaveRoomButton = Build<ButtonSprite>::create("Leave room", "bigFont.fnt", "GJ_button_01.png", 0.8f)
            .intoMenuItem([this](auto) {
                if (!this->isLoading()) {
                    NetworkManager::get().send(LeaveRoomPacket::create());
                    this->reloadPlayerList(false);
                }
            })
            .scaleMult(1.1f)
            .id("leave-room-btn"_spr)
            .parent(roomBtnMenu)
            .collect();
    }
}

void RoomLayer::removeLoadingCircle() {
    if (loadingCircle) {
        loadingCircle->fadeAndRemove();
        loadingCircle = nullptr;
    }
}

void RoomLayer::reloadPlayerList(bool sendPacket) {
    auto& nm = NetworkManager::get();
    if (!nm.established()) {
        // this->onClose(this);
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
    loadingCircle->ignoreAnchorPointForPosition(false);
    loadingCircle->setPosition(listLayer->getScaledContentSize() / 2.f);
    loadingCircle->show();

    this->recreateInviteButton();

    buttonMenu->updateLayout();
}

bool RoomLayer::isLoading() {
    return loadingCircle != nullptr;
}

void RoomLayer::sortPlayerList() {
    auto& flm = FriendListManager::get();

    // filter out the weird people (old game server used to send unauthenticated people too)
    filteredPlayerList.erase(std::remove_if(filteredPlayerList.begin(), filteredPlayerList.end(), [](const auto& player) {
        return player.accountId == 0;
    }), filteredPlayerList.end());

    // show friends before everyone else, and sort everyone alphabetically by the name
    std::sort(filteredPlayerList.begin(), filteredPlayerList.end(), [&flm](const auto& p1, const auto& p2) -> bool {
        auto selfId = GJAccountManager::get()->m_accountID;
        if (p1.accountId == selfId) return true;
        else if (p2.accountId == selfId) return false;

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

void RoomLayer::applyFilter(const std::string_view input) {
    filteredPlayerList.clear();

    if (input.empty()) {
        for (const auto& item : playerList) {
            filteredPlayerList.push_back(item);
        }

        clearSearchButton->removeFromParent();
        return;
    }

    auto filt = util::format::toLowercase(input);

    for (const auto& item : playerList) {
        auto name = util::format::toLowercase(item.name);
        if (name.find(filt) != std::string::npos) {
            filteredPlayerList.push_back(item);
        }
    }

    if (!clearSearchButton->getParent()) {
        buttonMenu->addChild(clearSearchButton);
    }

    buttonMenu->updateLayout();
}

void RoomLayer::setRoomTitle(std::string name, uint32_t id) {
    if (roomIdButton) roomIdButton->removeFromParent();

    std::string title;
    if (id == 0) {
        title = "Global room";
    } else {
        title = fmt::format("{} ({})", name, id);
    }

    auto layout = util::ui::getPopupLayout(popupSize);

    CCNode* elem = Build<CCLabelBMFont>::create(title.c_str(), "goldFont.fnt")
        .limitLabelWidth(listWidth, 0.7f, 0.2f)
        .collect();

    if (id != 0) {
        auto* button = CCMenuItemSpriteExtra::create(elem, this, menu_selector(RoomLayer::onCopyRoomId));
        button->addChild(elem);

        auto* menu = CCMenu::create();
        menu->addChild(button);

        elem = menu;
    }

    elem->setPosition(layout.centerTop - CCPoint{0.f, 17.f});
    this->addChild(elem);

    roomIdButton = elem;
}

void RoomLayer::onCopyRoomId(cocos2d::CCObject*) {
    auto id = RoomManager::get().getId();

    utils::clipboard::write(std::to_string(id));

    Notification::create("Copied to clipboard", NotificationIcon::Success)->show();
}

void RoomLayer::recreateInviteButton() {
    inviteButton->removeFromParent();

    auto& rm = RoomManager::get();

    if (rm.isInRoom()) {
        bool canInvite = rm.getInfo().settings.flags.publicInvites || rm.isOwner();

        if (canInvite) {
            buttonMenu->addChild(inviteButton);
        }
    }

    buttonMenu->updateLayout();
}

RoomLayer* RoomLayer::create() {
    auto ret = new RoomLayer();
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}
