#include "GlobedMenuLayer.hpp"
#include <globed/core/SettingsManager.hpp>
#include <globed/core/RoomManager.hpp>
#include <globed/core/PopupManager.hpp>
#include <globed/core/net/NetworkManager.hpp>
#include <globed/core/ServerManager.hpp>
#include <globed/core/FriendListManager.hpp>
#include <globed/core/actions.hpp>
#include <globed/util/Random.hpp>
#include <globed/util/GameState.hpp>
#include <core/net/NetworkManagerImpl.hpp>
#include <ui/misc/PlayerListCell.hpp>
#include <ui/admin/ModUserPopup.hpp>
#include <ui/misc/InputPopup.hpp>
#include <ui/menu/RoomListingPopup.hpp>
#include <ui/menu/ServerListPopup.hpp>
#include <ui/menu/CreateRoomPopup.hpp>
#include <ui/menu/RegionSelectPopup.hpp>
#include <ui/menu/RoomUserControlsPopup.hpp>
#include <ui/menu/TeamManagementPopup.hpp>
#include <ui/menu/InvitePopup.hpp>
#include <ui/menu/RoomSettingsPopup.hpp>
#include <ui/menu/SupportPopup.hpp>
#include <ui/menu/CreditsPopup.hpp>
#include <ui/menu/FeaturedPopup.hpp>
#include <ui/menu/UserSettingsPopup.hpp>
#include <ui/menu/PinnedLevelCell.hpp>
#include <ui/menu/levels/LevelListLayer.hpp>
#include <ui/settings/SettingsLayer.hpp>
#include <ui/misc/Badges.hpp>
#include <ui/misc/CellGradients.hpp>

#include <argon/argon.hpp>
#include <cue/RepeatingBackground.hpp>
#include <UIBuilder.hpp>

using namespace geode::prelude;
using namespace asp::time;

static constexpr CCSize PLAYER_LIST_MENU_SIZE{420.f, 290.f};
static constexpr CCSize PLAYER_LIST_SIZE{PLAYER_LIST_MENU_SIZE.width * 0.8f, 198.f};
static constexpr float CELL_HEIGHT = 27.f;
static constexpr CCSize CELL_SIZE{PLAYER_LIST_SIZE.width, CELL_HEIGHT};

static constexpr CCSize FAR_BTN_SIZE { 45.f, 45.f };

static constexpr float CONNECT_MENU_WIDTH = 260.f;

namespace globed {
namespace {
class PlayerCell : public PlayerListCell {
public:
    static PlayerListCell* create(
        int accountId,
        int userId,
        const std::string& username,
        const cue::Icons& icons,
        const std::optional<SpecialUserData>& sud,
        SessionId sessionId
    ) {
        auto ret = new PlayerCell();
        ret->m_sessionId = sessionId;

        if (ret->init(accountId, userId, username, icons, sud, CELL_SIZE)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }

    static PlayerListCell* createMyself(SessionId sessionId) {
        auto ret = new PlayerCell();
        ret->m_sessionId = sessionId;

        if (ret->initMyself(CELL_SIZE)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }

    void softRefresh(const RoomPlayer& rp) {
        if (rp.specialUserData) {
            m_nameLabel->updateWithRoles(*rp.specialUserData);
        } else {
            m_nameLabel->updateNoRoles();
        }

        m_leftContainer->updateLayout();

        if (rp.session != m_sessionId) {
            m_sessionId = rp.session;
            this->recreateSessionButton();
        }

        this->initGradients();
    }

protected:
    SessionId m_sessionId;
    CCMenuItemSpriteExtra* m_joinBtn = nullptr;
    static constexpr CCSize BTN_SIZE { 22.f, 22.f };

    bool customSetup() {
        this->recreateSessionButton();

        // if we are room host or a moderator, add a button that allows us kicking/banning the person or opening mod panel
        auto& rm = RoomManager::get();
        bool isMod = NetworkManagerImpl::get().isAuthorizedModerator();
        bool isOwner = rm.isOwner();
        bool isInRoom = !rm.isInGlobal();

        if (isInRoom && (isMod || isOwner)) {
            Build<CCSprite>::createSpriteName("GJ_reportBtn_001.png")
                .with([&](auto spr) { cue::rescaleToMatch(spr, BTN_SIZE); })
                .intoMenuItem([this] {
                    this->openUserControls();
                })
                .zOrder(9)
                .scaleMult(1.1f)
                .parent(m_rightMenu);
        } else if (isMod) {
            Build<CCSprite>::createSpriteName("GJ_reportBtn_001.png")
                .with([&](auto spr) { cue::rescaleToMatch(spr, BTN_SIZE); })
                .intoMenuItem([this] {
                    this->openModPanel();
                })
                .zOrder(9)
                .scaleMult(1.1f)
                .parent(m_rightMenu);
        }

        m_rightMenu->updateLayout();

        this->initGradients();

        return true;
    }

    void recreateSessionButton() {
        cue::resetNode(m_joinBtn);

        if (m_sessionId.asU64() != 0) {
            // add button to join the session
            m_joinBtn = Build<CCSprite>::createSpriteName("GJ_playBtn2_001.png")
                .with([&](auto spr) { cue::rescaleToMatch(spr, BTN_SIZE); })
                .intoMenuItem([this] {
                    globed::warpToSession(m_sessionId, false, true);
                })
                .zOrder(10)
                .scaleMult(1.1f)
                .parent(m_rightMenu);
        }

        m_rightMenu->updateLayout();
    }

    void openUserControls() {
        RoomUserControlsPopup::create(m_accountId, m_username)->show();
    }

    void openModPanel() {
        ModUserPopup::create(m_accountId)->show();
    }
};
} // namespace

// order of buttons in right side menu
namespace RightBtn {
    constexpr int PrivacySettings = 3;
    constexpr int AdminPanel = 4;
    constexpr int Search = 5;
    constexpr int ClearSearch = 6;
    constexpr int Invite = 7;
    constexpr int Refresh = 100; // dead last
}

namespace LeftBtn {
    constexpr int Disconnect = 100;
    constexpr int RegionSwitch = 110;
    constexpr int Teams = 300;
    constexpr int Settings = 400;
    constexpr int CloseRoom = 500;
}

namespace FarRightBtn {
    constexpr int Discord = 100;
    constexpr int Support = 200;
    constexpr int Credits = 300;
}

namespace FarLeftBtn {
    constexpr int Levels = 300;
    constexpr int Settings = 99999; // dead last
}

bool GlobedMenuLayer::init() {
    if (!BaseLayer::init(false)) return false;

    Build<cue::RepeatingBackground>::create("game_bg_01_001.png")
        .id("background")
        .color({37, 50, 167})
        .parent(this)
        .as<CCSprite>()
        .store(m_background);

    auto winSize = CCDirector::get()->getWinSize();

    // add a small version label in top right
    Build<Label>::create(fmt::format("{}", Mod::get()->getVersion().toVString()), "chatFont.fnt")
        .opacity(127)
        .scale(0.55f)
        .anchorPoint(1.f, 1.f)
        .pos(winSize - CCSize{1.f, 1.f})
        .zOrder(111)
        .parent(this);

    // create the connection menu
    auto connectMenuLayout = ColumnLayout::create()->setAutoScale(false)->setAxisReverse(true)->setGap(10.f);
    connectMenuLayout->ignoreInvisibleChildren(true);
    m_connectMenu = Build<CCMenu>::create()
        .id("connect-menu")
        .layout(connectMenuLayout)
        .pos(winSize.width / 2.f, winSize.height * 0.5f)
        .contentSize(CONNECT_MENU_WIDTH, 150.f)
        .anchorPoint(0.5f, 0.5f)
        .parent(this);

    m_connectMenuBg = cue::attachBackground(m_connectMenu, cue::BackgroundOptions {
        .opacity = 255,
        .texture = "GJ_square01.png",
    });

    auto serverFieldLayout = RowLayout::create()->setAutoScale(false);
    serverFieldLayout->ignoreInvisibleChildren(true);
    Build<CCMenu>::create()
        .id("server-field")
        .layout(serverFieldLayout)
        .child(
            Build<CCLabelBMFont>::create("Server Name", "goldFont.fnt")
                .store(m_serverNameLabel)
        )
        .child(
            Build<CCSprite>::create("pencil.png"_spr)
                .scale(0.7f)
                .intoMenuItem([this] {
                    ServerListPopup::create(this)->show();
                })
                .store(m_editServerButton)
        )
        .contentSize(CONNECT_MENU_WIDTH, 40.f)
        .updateLayout()
        .parent(m_connectMenu);

    m_connectButton = Build<ButtonSprite>::create("Connect", "bigFont.fnt", "GJ_button_01.png", 0.7f)
        .scale(0.9f)
        .intoMenuItem([] {
            // don't allow if not logged into an account
            if (!argon::signedIn()) {
                globed::confirmPopup(
                    "Globed Error",
                    "You must be logged into a <cg>Geometry Dash account</c> in order to play online. Want to visit the <cy>account page</c>?",
                    "Cancel", "Ok",
                    [](auto) {
                        AccountLayer::create()->showLayer(false);
                    }
                );
                return;
            }

            auto& sm = ServerManager::get();
            auto url = sm.getActiveServer().url;

            if (auto err = NetworkManager::get().connectCentral(url).err()) {
                log::error("failed to connect to central server: {}", err);
                return;
            }
        })
        .scaleMult(1.1f)
        .parent(m_connectMenu);

    auto cscLayout = RowLayout::create()->setAutoScale(false);
    cscLayout->ignoreInvisibleChildren(true);
    m_connStateContainer = Build<CCMenu>::create()
        .layout(cscLayout)
        .contentSize(240.f, 28.f)
        .parent(m_connectMenu)
        .id("conn-state-container")
        .collect();

    // connection state label
    m_connStateLabel = Build<CCLabelBMFont>::create("", "bigFont.fnt")
        .scale(0.6f)
        .id("conn-state-lbl")
        .parent(m_connStateContainer);

    // cancel connection button
    m_cancelConnButton = Build<CCSprite>::create("exit01.png"_spr)
        .with([&](auto spr) { cue::rescaleToMatch(spr, 27.5f); })
        .intoMenuItem([] {
            NetworkManagerImpl::get().disconnectCentral();
        })
        .id("cancel-conn-btn")
        .parent(m_connStateContainer);

    m_connStateContainer->updateLayout();

    // button menu
    auto buttonMenu = Build<CCMenu>::create()
        .contentSize(CONNECT_MENU_WIDTH, 40.f)
        .layout(RowLayout::create()->setAutoScale(false))
        .parent(m_connectMenu)
        .collect();

    Build<CCSprite>::create("settings01.png"_spr)
        .with([&](auto spr) { cue::rescaleToMatch(spr, FAR_BTN_SIZE); })
        .intoMenuItem([this] {
            this->onSettings();
        })
        .scaleMult(1.1f)
        .parent(buttonMenu);

    for (auto btn : this->createCommonButtons()) {
        buttonMenu->addChild(btn);
    }

    buttonMenu->updateLayout();

    // player list menu

    m_playerListMenu = Build<CCMenu>::create()
        .id("player-list-menu")
        .ignoreAnchorPointForPos(false)
        .contentSize(PLAYER_LIST_MENU_SIZE)
        .pos(winSize / 2.f)
        .anchorPoint(0.5f, 0.5f)
        .parent(this)
        .visible(false);

    Build(CCScale9Sprite::create("GJ_square02.png", {0, 0, 80, 80}))
        .contentSize(PLAYER_LIST_MENU_SIZE)
        .pos(PLAYER_LIST_MENU_SIZE / 2.f)
        .parent(m_playerListMenu);

    m_roomNameButton = Build<Label>::create("", "goldFont.fnt")
        .scale(0.7f)
        .store(m_roomNameLabel)
        .intoMenuItem([this] {
            this->copyRoomIdToClipboard();
        })
        .scaleMult(1.1f)
        .pos(PLAYER_LIST_MENU_SIZE.width / 2.f, PLAYER_LIST_MENU_SIZE.height - 16.f)
        .id("room-name-btn")
        .parent(m_playerListMenu);

    m_playerList = Build<cue::ListNode>::create(PLAYER_LIST_SIZE, ccColor4B{0x33, 0x44, 0x99, 255}, cue::ListBorderStyle::CommentsBlue)
        .anchorPoint(0.5f, 1.f)
        .pos(PLAYER_LIST_MENU_SIZE.width / 2.f, PLAYER_LIST_MENU_SIZE.height - 36.f)
        .parent(m_playerListMenu);

    m_playerList->setJustify(cue::Justify::Center);
    m_playerList->setCellColors(
        ccColor4B{0x28, 0x35, 0x77, 255},
        ccColor4B{0x33, 0x44, 0x99, 255}
    );

    m_roomButtonsMenu = Build<CCMenu>::create()
        .id("room-buttons")
        .layout(RowLayout::create()->setAutoScale(false))
        .contentSize(PLAYER_LIST_SIZE.width, 64.f)
        .pos(PLAYER_LIST_MENU_SIZE.width / 2.f, 30.f)
        .parent(m_playerListMenu);

    auto colLayout = ColumnLayout::create()
        ->setAutoScale(false)
        ->setGap(3.f)
        ->setAxisReverse(true)
        ->setAxisAlignment(AxisAlignment::End);
    colLayout->ignoreInvisibleChildren(true);

    m_leftSideMenu = Build<CCMenu>::create()
        .id("left-side-menu")
        .layout(colLayout)
        .contentSize(PLAYER_LIST_MENU_SIZE.width * 0.08f, PLAYER_LIST_MENU_SIZE.height - 12.f)
        .pos(7.f, PLAYER_LIST_MENU_SIZE.height - 8.f)
        .anchorPoint(0.f, 1.f)
        .parent(m_playerListMenu);

    m_rightSideMenu = Build<CCMenu>::create()
        .id("right-side-menu")
        .layout(colLayout)
        .contentSize(PLAYER_LIST_MENU_SIZE.width * 0.08f, PLAYER_LIST_MENU_SIZE.height - 12.f)
        .pos(PLAYER_LIST_MENU_SIZE - CCSize{7.f, 8.f})
        .anchorPoint(1.f, 1.f)
        .parent(m_playerListMenu);

    // init far menus

    auto farLayout = ColumnLayout::create()
        ->setAutoScale(false)
        ->setAxisReverse(true)
        ->setAxisAlignment(AxisAlignment::Start);

    m_farLeftMenu = Build<CCMenu>::create()
        .id("far-left-menu")
        .layout(farLayout)
        .contentSize(48.f, 250.f)
        .pos(16.f, 18.f)
        .anchorPoint(0.f, 0.f)
        .parent(this);

    m_farRightMenu = Build<CCMenu>::create()
        .id("far-right-menu")
        .layout(farLayout)
        .contentSize(48.f, 250.f)
        .pos(winSize.width - 16.f, 18.f)
        .anchorPoint(1.f, 0.f)
        .parent(this);

    auto& nm = NetworkManagerImpl::get();

    m_roomStateListener = nm.listen<msg::RoomStateMessage>([this](const auto& msg) {
        if (msg.roomId != m_roomId) {
            this->initNewRoom(msg.roomId, msg.roomName, msg.players, msg.playerCount, msg.settings);
        } else {
            this->updateRoom(msg.roomId, msg.roomName, msg.players, msg.playerCount, msg.settings);
        }

        return ListenerResult::Continue;
    });

    m_roomPlayersListener = nm.listen<msg::RoomPlayersMessage>([this](const auto& msg) {
        this->updatePlayerList(msg.players);

        return ListenerResult::Continue;
    });

    m_pinnedListener = nm.listen<msg::PinnedLevelUpdatedMessage>([this](const auto& msg) {
        this->addPinnedLevelCell();

        return ListenerResult::Continue;
    });

    this->onServerModified();

    this->update(0.f);
    this->scheduleUpdate();

    return true;
}

void GlobedMenuLayer::initNewRoom(uint32_t id, const std::string& name, const std::vector<RoomPlayer>& players, size_t playerCount, const RoomSettings& settings) {
    m_roomId = id;

    this->updateRoom(id, name, players, playerCount, settings);
    this->initRoomButtons();
    this->initSideButtons();
}

void GlobedMenuLayer::updateRoom(uint32_t id, const std::string& name, const std::vector<RoomPlayer>& players, size_t playerCount, const RoomSettings& settings) {
    bool hideId = globed::setting<bool>("core.streamer-mode");

    std::string labelText;

    if (id == 0) {
        labelText = fmt::format("{} ({} {})", name, playerCount, playerCount == 1 ? "player" : "players");
    } else if (hideId) {
        labelText = name;
    } else {
        labelText = fmt::format("{} ({})", name, id);
    }

    if (m_roomNameLabel->getString() != labelText) {
        m_roomNameLabel->setString(labelText);
        m_roomNameLabel->limitLabelWidth(320.f, 0.7f, 0.35f);
        auto size = m_roomNameLabel->getContentSize();

        m_roomNameLabel->setPosition(size / 2.f);
        m_roomNameButton->setContentSize(size);
    }

    this->updatePlayerList(players);
}

void GlobedMenuLayer::updatePlayerList(const std::vector<RoomPlayer>& players) {
    m_playerCount = players.size();

    if (!m_hardRefresh) {
        if (this->trySoftRefresh(players)) {
            return;
        }
    }

    m_hardRefresh = false;

    auto scrollPos = m_playerList->getScrollPos();

    m_playerList->setAutoUpdate(false);
    m_playerList->clear();

    CCSize cellSize{PLAYER_LIST_SIZE.width, CELL_HEIGHT};

    auto& flm = FriendListManager::get();
    auto& rm = RoomManager::get();
    bool randomize = rm.isInGlobal();
    int roomOwnerId = rm.getRoomOwner();
    int selfId = cachedSingleton<GJAccountManager>()->m_accountID;
    std::vector<RoomPlayer> sortedPlayers = players;

    // sort all the players
    std::sort(sortedPlayers.begin(), sortedPlayers.end(), [&](auto& a, auto& b) {
        // The order is as follows:
        // 1. Room owner
        // 2. Local player
        // 3. Friends (sorted alphabetically)
        // 4. everyone else, sorted either alphabetically or shuffled

        bool isAOwner = a.accountData.accountId == roomOwnerId;
        bool isBOwner = b.accountData.accountId == roomOwnerId;
        if (isAOwner != isBOwner) return isAOwner;

        bool isALocal = a.accountData.accountId == selfId;
        bool isBLocal = b.accountData.accountId == selfId;
        if (isALocal != isBLocal) return isALocal;

        bool isAFriend = flm.isFriend(a.accountData.accountId);
        bool isBFriend = flm.isFriend(b.accountData.accountId);
        if (isAFriend != isBFriend) return isAFriend;

        // proper alphabetical sorting requires copying the usernames and converting them to lowercase,
        // only do it if we wont end up shuffling at the end
        if ((isAFriend && isBFriend) || !randomize) {
            // convert both names to lowercase
            std::string name1 = a.accountData.username, name2 = b.accountData.username;
            std::transform(name1.begin(), name1.end(), name1.begin(), ::tolower);
            std::transform(name2.begin(), name2.end(), name2.begin(), ::tolower);

            return name1 < name2;
        }

        return a.accountData.userId < b.accountData.userId;
    });

    if (randomize) {
        // find first element thats not forced at the top
        decltype(sortedPlayers)::iterator firstNonFriend = sortedPlayers.end();

        for (auto it = sortedPlayers.begin(); it != sortedPlayers.end(); it++) {
            int accountId = it->accountData.accountId;
            if (accountId != roomOwnerId && accountId != selfId && !flm.isFriend(accountId)) {
                firstNonFriend = it;
                break;
            }
        }

        // shuffle everything afterwards
        std::shuffle(firstNonFriend, sortedPlayers.end(), rng()->engine());
    }

    this->addPinnedLevelCell();

    m_playerList->addCell(PlayerCell::createMyself(SessionId{}));

    for (auto& player : sortedPlayers) {
        m_playerList->addCell(PlayerCell::create(
            player.accountData.accountId,
            player.accountData.userId,
            player.accountData.username,
            cue::Icons {
                .type = IconType::Cube,
                .id = player.cube,
                .color1 = player.color1.asIdx(),
                .color2 = player.color2.asIdx(),
                .glowColor = player.glowColor.asIdx(),
            },
            player.specialUserData,
            player.session
        ));
    }

    m_playerList->setAutoUpdate(true);
    m_playerList->updateLayout(false);
    m_playerList->setScrollPos(scrollPos);

    cocos::handleTouchPriority(this, true);
}

void GlobedMenuLayer::addPinnedLevelCell() {
    // if there is no pinned level, remove instead
    auto id = RoomManager::get().getPinnedLevel().levelId();
    if (id == 0) {
        this->removePinnedLevelCell();
        return;
    }

    PinnedLevelCell* cell = nullptr;
    if (m_playerList->size() > 0) {
        auto lcell = m_playerList->getCell(0);
        cell = typeinfo_cast<PinnedLevelCell*>(lcell->getInner());
    }

    if (!cell) {
        cell = PinnedLevelCell::create(CELL_SIZE.width);
        cell->setUpdateCallback([this] {
            m_playerList->updateLayout();
        });

        m_playerList->insertCell(cell, 0);
    }

    cell->loadLevel(id);
}

void GlobedMenuLayer::removePinnedLevelCell() {
    if (m_playerList->size() == 0) {
        return;
    }

    auto cell = m_playerList->getCell(0);
    if (typeinfo_cast<PinnedLevelCell*>(cell->getInner())) {
        m_playerList->removeCell(cell);
    }
}

bool GlobedMenuLayer::trySoftRefresh(const std::vector<RoomPlayer>& players) {
    if (m_playerCount == 0) return false;

    this->addPinnedLevelCell();

    auto selfId = globed::cachedSingleton<GJAccountManager>()->m_accountID;

    auto scrollPos = m_playerList->getScrollPos();

    std::unordered_map<int, const RoomPlayer*> newp;
    std::unordered_map<int, PlayerCell*> existing;

    for (auto cell : m_playerList->iterChecked<PlayerCell>()) {
        if (cell->m_accountId == selfId) continue;

        existing[cell->m_accountId] = cell;
    }

    for (auto& pl : players) {
        if (pl.accountData.accountId == selfId) continue;

        newp[pl.accountData.accountId] = &pl;

        // safe refresh not done if there are any new players
        if (!existing.contains(pl.accountData.accountId)) {
            return false;
        }
    }

    // remove any players that left, update existing players
    for (auto it = existing.begin(); it != existing.end();) {
        auto& [player, cell] = *it;
        auto newit = newp.find(player);

        if (newit == newp.end()) {
            auto idx = m_playerList->indexForCell((cue::ListCell*)cell->getParent());
            m_playerList->removeCell(idx);
            it = existing.erase(it);
        } else {
            // soft refresh on the player
            cell->softRefresh(*newit->second);

            it++;
        }
    }

    m_playerList->updateLayout(false);
    m_playerList->setScrollPos(scrollPos);

    return true;
}

void GlobedMenuLayer::initRoomButtons() {
    // fix cocos ub, i hate it here
    m_roomButtonsMenu->m_pSelectedItem = nullptr;
    m_roomButtonsMenu->removeAllChildren();

    constexpr float BtnScale = 0.77f;

    if (m_roomId == 0) {
        // global room, show buttons to create / join a room

        Build(ButtonSprite::create("Join Room", "bigFont.fnt", "GJ_button_01.png", BtnScale))
            .intoMenuItem(+[] {
                if (auto p = RoomListingPopup::create()) p->show();
            })
            .scaleMult(1.1f)
            .parent(m_roomButtonsMenu);

        Build(ButtonSprite::create("Create Room", "bigFont.fnt", "GJ_button_01.png", BtnScale))
            .intoMenuItem(+[] {
                CreateRoomPopup::create()->show();
            })
            .scaleMult(1.1f)
            .parent(m_roomButtonsMenu);
    } else {
        Build(ButtonSprite::create("Leave Room", "bigFont.fnt", "GJ_button_01.png", BtnScale))
            .intoMenuItem(+[] {
                NetworkManagerImpl::get().sendLeaveRoom();
            })
            .scaleMult(1.1f)
            .parent(m_roomButtonsMenu);
    }

    m_roomButtonsMenu->updateLayout();
}

void GlobedMenuLayer::initSideButtons() {
    m_rightSideMenu->removeAllChildren();
    m_leftSideMenu->removeAllChildren();

    constexpr static CCSize buttonSize {30.f, 30.f};

    auto makeButton = [](CCSprite* sprite, CCNode* parent, int zOrder, const char* id, auto cb) -> CCMenuItemSpriteExtra* {
        if (!sprite) {
            log::error("Sprite is null for {}!", id);
            return nullptr;
        }

        return Build(sprite)
            .with([&](auto btn) { cue::rescaleToMatch(btn, buttonSize); })
            .intoMenuItem([cb = std::move(cb)](auto) {
                cb();
            })
            .zOrder(zOrder)
            .scaleMult(1.1f)
            .id(id)
            .parent(parent)
            .collect();
    };

    auto& rm = RoomManager::get();

    /// Left side buttons

    makeButton(
        CCSprite::create("exit02.png"_spr),
        m_leftSideMenu,
        LeftBtn::Disconnect,
        "btn-disconnect",
        [] {
            globed::confirmPopup(
                "Note",
                "Are you sure you want to <cr>disconnect</c> from the server?",
                "Cancel", "Yes",
                [](auto) {
                    NetworkManagerImpl::get().disconnectCentral();
                }
            );
        }
    );

    // region switching button
    makeButton(
        CCSprite::create("server02.png"_spr),
        m_leftSideMenu,
        LeftBtn::RegionSwitch,
        "btn-region-select",
        [] {
            if (auto popup = RegionSelectPopup::create()) {
                popup->show();

                if (!globed::swapFlag("core.flags.seen-region-select-note")) {
                    RegionSelectPopup::showInfo();
                }
            }
        }
    );

    if (rm.getSettings().teams) {
        makeButton(
            CCSprite::create("teams02.png"_spr),
            m_leftSideMenu,
            LeftBtn::Teams,
            "btn-manage-teams",
            [] {
                TeamManagementPopup::create(0)->show();
            }
        );
    }

    if (rm.isOwner()) {
        auto btn = makeButton(
            CCSprite::create("settings01.png"_spr),
            m_leftSideMenu,
            LeftBtn::Settings,
            "btn-settings",
            [] {
                RoomSettingsPopup::create()->show();
            }
        );
    }

    // close room button
    if (!rm.isInGlobal() && (rm.isOwner() || NetworkManagerImpl::get().isAuthorizedModerator())) {
        makeButton(
            CCSprite::create("exit01.png"_spr),
            m_leftSideMenu,
            LeftBtn::CloseRoom,
            "btn-close-room",
            [] {
                globed::confirmPopup(
                    "Close Room",
                    "Are you sure you want to <cr>close</c> the room? All players will be <cy>kicked</c> from the room and it will be <cy>deleted</c>.",
                    "Cancel", "Ok",
                    [](auto) {
                        NetworkManagerImpl::get().sendRoomOwnerAction(RoomOwnerActionType::CLOSE_ROOM);
                    }
                );
            }
        );
    }

    /// Right side buttons

    // user settings button
    makeButton(
        CCSprite::create("privacySettings.png"_spr),
        m_rightSideMenu,
        RightBtn::PrivacySettings,
        "btn-privacy-settings",
        [] {
            UserSettingsPopup::create()->show();
        }
    );

    // mod panel button
    if (NetworkManagerImpl::get().isModerator()) {
        auto badge = globed::createMyBadge();
        if (!badge) {
            badge = globed::createBadge("role-unknown.png");
        }

        if (badge) {
            auto wrapper = CCSprite::create("aquaBlank.png"_spr);
            badge->setPosition(wrapper->getContentSize() / 2.f + CCPoint{0.f, 1.f});
            cue::rescaleToMatch(badge, wrapper->getContentWidth() * 0.7f);
            wrapper->addChild(badge);

            makeButton(
                wrapper,
                m_rightSideMenu,
                RightBtn::AdminPanel,
                "btn-managemod-panel",
                [] { globed::openModPanel(); }
            );

            badge->setScale(badge->getScale() * 0.85f);
        } else {
            log::error("Failed to create mod panel button, badge not found");
        }
    }

    // invite button
    if (!rm.isInGlobal() && (rm.isOwner() || !rm.getSettings().privateInvites)) {
        makeButton(
            CCSprite::create("invite.png"_spr),
            m_rightSideMenu,
            RightBtn::Invite,
            "btn-invite",
            [] {
                InvitePopup::create()->show();
            }
        );
    }

    // filter button
    m_searchBtn = makeButton(
        CCSprite::create("search01.png"_spr),
        m_rightSideMenu,
        RightBtn::Search,
        "btn-search",
        [this] {
            auto popup = InputPopup::create("bigFont.fnt");
            popup->setTitle("Search Player");
            popup->setPlaceholder("Username");
            popup->setMaxCharCount(16);
            popup->setCommonFilter(CommonFilter::Name);
            popup->setWidth(240.f);
            popup->setCallback([this](auto outcome) {
                if (outcome.cancelled) return;

                this->reloadWithFilter(outcome.text);
            });
            popup->show();
        }
    );

    // clear filter button
    m_clearSearchBtn = makeButton(
        CCSprite::create("search02.png"_spr),
        m_rightSideMenu,
        RightBtn::ClearSearch,
        "btn-clear-search",
        [this] {
            this->reloadWithFilter("");
        }
    );
    m_clearSearchBtn->setVisible(false);

    // refresh button
    makeButton(
        CCSprite::create("refresh01.png"_spr),
        m_rightSideMenu,
        RightBtn::Refresh,
        "btn-refresh",
        [this] {
            m_hardRefresh = true;

            if (m_curFilter.empty()) {
                this->requestRoomState();
            } else {
                NetworkManagerImpl::get().sendRequestRoomPlayers(m_curFilter);
            }
        }
    );

    m_rightSideMenu->updateLayout();
    m_leftSideMenu->updateLayout();
}

void GlobedMenuLayer::initFarSideButtons() {
    auto winSize = CCDirector::get()->getWinSize();
    m_farLeftMenu->removeAllChildren();
    m_farRightMenu->removeAllChildren();

    bool connected = m_state == MenuState::Connected;

    if (!connected) {
        m_farLeftMenu->updateLayout();
        m_farRightMenu->updateLayout();
        return;
    }

    auto commons = this->createCommonButtons();
    for (auto b : commons) {
        m_farRightMenu->addChild(b);
    }

    Build<CCSprite>::create("settings01.png"_spr)
        .with([&](auto btn) { cue::rescaleToMatch(btn, FAR_BTN_SIZE); })
        .intoMenuItem([this] {
            this->onSettings();
        })
        .scaleMult(1.1f)
        .zOrder(FarLeftBtn::Settings)
        .parent(m_farLeftMenu);

    Build<CCSprite>::create("levels01.png"_spr)
        .with([&](auto btn) { cue::rescaleToMatch(btn, FAR_BTN_SIZE); })
        .intoMenuItem([] {
            LevelListLayer::create()->switchTo();
        })
        .scaleMult(1.1f)
        .zOrder(FarLeftBtn::Levels)
        .parent(m_farLeftMenu);

    auto& nm = NetworkManagerImpl::get();
    auto flevel = nm.getFeaturedLevel();

    if (flevel) {
        bool isNew = !nm.hasViewedFeaturedLevel();

        auto fbutton = Build<CCSprite>::create("feature01.png"_spr)
            .with([&](auto btn) { cue::rescaleToMatch(btn, FAR_BTN_SIZE); })
            .intoMenuItem(+[](CCMenuItemSpriteExtra* self) {
                FeaturedPopup::create()->show();

                // make it not new
                NetworkManagerImpl::get().setViewedFeaturedLevel();

                if (auto spr = self->getChildByID("btn-daily-extra"_spr)) {
                    spr->setVisible(false);
                }
            })
            .scaleMult(1.1f)
            .zOrder(FarLeftBtn::Levels - 1)
            .parent(m_farLeftMenu)
            .collect();

        if (isNew) {
            Build<CCSprite>::createSpriteName("newMusicIcon_001.png")
                .id("btn-daily-extra"_spr)
                .anchorPoint({0.5, 0.5})
                .pos({fbutton->getScaledContentWidth() * 0.85f, fbutton->getScaledContentHeight() * 0.15f})
                .zOrder(2)
                .visible(false)
                .parent(fbutton)
                .with([&](auto spr) {
                    spr->runAction(
                        CCRepeatForever::create(CCSequence::create(
                            CCEaseSineInOut::create(CCScaleTo::create(0.75f, 1.2f)),
                            CCEaseSineInOut::create(CCScaleTo::create(0.75f, 1.0f)),
                            nullptr
                        ))
                    );
                });
        }
    }

    m_farLeftMenu->updateLayout();
    m_farRightMenu->updateLayout();
}

std::vector<Ref<CCMenuItemSpriteExtra>> GlobedMenuLayer::createCommonButtons() {
    std::vector<Ref<CCMenuItemSpriteExtra>> out;

    // credits
    out.push_back(Build<CCSprite>::create("support01.png"_spr)
        .with([&](auto btn) { cue::rescaleToMatch(btn, FAR_BTN_SIZE); })
        .intoMenuItem([] {
            CreditsPopup::create()->show();
        })
        .scaleMult(1.1f)
        .zOrder(FarRightBtn::Credits)
        .collect()
    );

    // supporter popup
    out.push_back(Build<CCSprite>::create("support02.png"_spr)
        .with([&](auto btn) { cue::rescaleToMatch(btn, FAR_BTN_SIZE); })
        .intoMenuItem([] {
            SupportPopup::create()->show();
        })
        .scaleMult(1.1f)
        .zOrder(FarRightBtn::Support)
        .collect()
    );

    // discord
    out.push_back(Build<CCSprite>::create("discord01.png"_spr)
        .with([&](auto btn) { cue::rescaleToMatch(btn, FAR_BTN_SIZE); })
        .intoMenuItem([] {
            globed::confirmPopup(
                "Open Discord",
                "Join our <cp>Discord</c> server?\n\n<cr>Important: By joining the Discord server, you agree to being at least 13 years of age.</c>",
                "No", "Yes",
                [](auto) {
                    geode::utils::web::openLinkInBrowser(globed::constant<"discord">());
                }
            );
        })
        .scaleMult(1.1f)
        .zOrder(FarRightBtn::Discord)
        .collect()
    );

    return out;
}

void GlobedMenuLayer::copyRoomIdToClipboard() {
    if (m_roomId == 0) return;

    geode::utils::clipboard::write(fmt::to_string(m_roomId));
    globed::toastSuccess("Copied room ID to clipboard!");
}

void GlobedMenuLayer::requestRoomState() {
    m_lastRoomUpdate = Instant::now();
    NetworkManagerImpl::get().sendRoomStateCheck();
}

bool GlobedMenuLayer::shouldAutoRefresh(float dt) {
    if (!m_curFilter.empty()) {
        return false;
    }

    m_autoRefreshCounter += dt;

    // Current game state check is somewhat expensive, prevent it being called too often
    if (m_autoRefreshCounter < 0.5f) {
        return false;
    }
    m_autoRefreshCounter = 0.f;

    auto state = globed::getCurrentGameState();
    auto playerCount = m_playerList->size();

    Duration intv;

    if (playerCount < 25) {
        intv = Duration::fromSecs(3);
    } else if (m_roomId != 0) {
        intv = Duration::fromSecs(10);
    } else {
        // intv = Duration::fromSecs(60);
        intv = Duration::fromSecs(3600); // dont really refresh in global room
    }

    // depending on the state, increase the interval
    if (state == GameState::Afk) {
        intv *= 6;
    } else if (state == GameState::Closed) {
        return false;
    }

    auto bytime = !m_lastRoomUpdate || m_lastRoomUpdate->elapsed() > intv;
    if (!bytime) {
        return false;
    }

    // dont refresh if the player interacted with the ui recently
    if (m_lastInteraction && m_lastInteraction->elapsed() < Duration::fromSecs(3)) {
        return false;
    }

    // dont refresh if the list is not at the top
    if (!m_playerList->isAtTop()) {
        return false;
    }

    // dont refresh if theres any popups covering the list
    for (auto child : CCScene::get()->getChildrenExt()) {
        if (typeinfo_cast<FLAlertLayer*>(child)) {
            return false;
        }
    }

    return true;
}

void GlobedMenuLayer::onSettings() {
    SettingsLayer::create()->switchTo();
}

void GlobedMenuLayer::onServerModified() {
    bool showEditBtn = globed::setting<bool>("core.ui.allow-custom-servers");
    m_editServerButton->setVisible(showEditBtn);

    // set the name of the server
    const auto& serverName = ServerManager::get().getActiveServer().name;
    m_serverNameLabel->setString(serverName.c_str());
    m_serverNameLabel->limitLabelWidth(180.f, 0.7f, 0.1f);

    m_serverNameLabel->getParent()->updateLayout();

    this->setMenuState(m_state, true);
}

void GlobedMenuLayer::update(float dt) {
    auto& nm = NetworkManager::get();
    auto connState = nm.getConnectionState();

    MenuState newState;
    switch (connState) {
        case ConnectionState::Disconnected: {
            newState = MenuState::Disconnected;
        } break;

        case ConnectionState::DnsResolving: {
            newState = MenuState::Connecting;
            m_connStateLabel->setString("DNS Resolving...");
        } break;

        case ConnectionState::Pinging: {
            newState = MenuState::Connecting;
            m_connStateLabel->setString("Pinging server...");
        } break;

        case ConnectionState::Connecting: {
            newState = MenuState::Connecting;
            m_connStateLabel->setString("Connecting...");
        } break;

        case ConnectionState::Authenticating: {
            newState = MenuState::Connecting;
            m_connStateLabel->setString("Authenticating...");
        } break;

        case ConnectionState::Connected: {
            newState = MenuState::Connected;
        } break;

        case ConnectionState::Closing: {
            newState = MenuState::Connecting;
            m_connStateLabel->setString("Disconnecting...");
        } break;

        case ConnectionState::Reconnecting: {
            newState = MenuState::Connecting;
            m_connStateLabel->setString("Reconnecting...");
        } break;

        default: {
            log::error("Unknown connection state: {}", static_cast<int>(connState));
            newState = MenuState::Disconnected;
        } break;
    }

    m_connStateLabel->limitLabelWidth(210.f, 0.7f, 0.2f);

    if (m_lastConnState != connState) {
        m_lastConnState = connState;
        m_connStateContainer->updateLayout();
    }

    this->setMenuState(newState);

    if (newState == MenuState::Connected) {
        auto scpos = m_playerList->getScrollPos();
        bool changed = !m_lastScrollPos || m_lastScrollPos.value() != scpos;
        m_lastScrollPos = scpos;

        if (changed) {
            m_lastInteraction = Instant::now();
        }

        if (m_roomId == (uint32_t)-1 && (!m_lastRoomUpdate || m_lastRoomUpdate->elapsed() > Duration::fromSecs(1))) {
            this->requestRoomState();
        } else if (this->shouldAutoRefresh(dt)) {
            this->requestRoomState();
        }
    }
}

void GlobedMenuLayer::setMenuState(MenuState state, bool force) {
    if (m_state == state && !force) return;
    m_state = state;

    switch (state) {
        case MenuState::None: break;

        case MenuState::Disconnected: {
            m_connectMenu->setVisible(true);
            m_editServerButton->setEnabled(true);
            m_connectButton->setVisible(true);
            m_connStateContainer->setVisible(false);
            m_playerListMenu->setVisible(false);
        } break;

        case MenuState::Connecting: {
            m_connectMenu->setVisible(true);
            m_editServerButton->setEnabled(false);
            m_connectButton->setVisible(false);
            m_connStateContainer->setVisible(true);
            m_playerListMenu->setVisible(false);
        } break;

        case MenuState::Connected: {
            m_connectMenu->setVisible(false);
            m_playerListMenu->setVisible(true);
            m_playerList->clear();
            m_hardRefresh = true;
            this->requestRoomState();
        } break;
    }

    m_cancelConnButton->setVisible(state == MenuState::Connecting);
    m_connStateContainer->updateLayout();

    this->initFarSideButtons();

    // yikes
    m_connectMenuBg->setVisible(false);
    m_connectMenu->updateLayout();
    m_connectMenuBg->setVisible(true);
}

void GlobedMenuLayer::keyDown(enumKeyCodes key) {
#ifdef GLOBED_DEBUG
    bool showDebug = true;
#else
    bool showDebug = globed::value<bool>("core.dev.enable-dev-settings").value_or(false);
#endif

    // a couple dev controls
    if (showDebug) {
        this->handleDebugKey(key);
    }

    BaseLayer::keyDown(key);
}

void GlobedMenuLayer::handleDebugKey(cocos2d::enumKeyCodes key) {
    // Debug keys:
    // F6 - simulate a connection breakage (for testing reconnects)
    // F7 - dump network stats (requires net stat dump setting to be enabled)
    switch (key) {
        case KEY_F6: {
            NetworkManagerImpl::get().simulateConnectionDrop();
        } break;

        case KEY_F7: {
            NetworkManagerImpl::get().dumpNetworkStats();
        } break;

        default: break;
    }
}

void GlobedMenuLayer::keyBackClicked() {
    auto& rm = RoomManager::get();
    if (!rm.isInFollowerRoom() || rm.isOwner()) {
        // only the owner of a follower room can leave to the main menu
        BaseLayer::keyBackClicked();
    } else {
        globed::alert(
            "Error",
            "You are in a follower room, you <cr>cannot</c> leave to the main menu. Only the room owner can choose which levels to play."
        );
    }
}

void GlobedMenuLayer::onEnter() {
    BaseLayer::onEnter();
    this->onServerModified();
}

void GlobedMenuLayer::reloadWithFilter(const std::string& filter) {
    m_curFilter = filter;

    auto& nm = NetworkManagerImpl::get();

    m_searchBtn->setVisible(filter.empty());
    m_clearSearchBtn->setVisible(!filter.empty());

    m_rightSideMenu->updateLayout();

    if (filter.empty()) {
        this->requestRoomState();
    } else {
        nm.sendRequestRoomPlayers(filter);
    }
}

bool GlobedMenuLayer::ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) {
    auto pos = this->convertTouchToNodeSpace(touch);
    if (CCRect{{}, this->getContentSize()}.containsPoint(pos)) {
        m_lastInteraction = Instant::now();
    }

    return CCLayer::ccTouchBegan(touch, event);
}

GlobedMenuLayer* GlobedMenuLayer::create() {
    auto ret = new GlobedMenuLayer();
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

}
