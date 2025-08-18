#include "GlobedMenuLayer.hpp"
#include <globed/core/SettingsManager.hpp>
#include <globed/core/RoomManager.hpp>
#include <globed/core/PopupManager.hpp>
#include <globed/core/net/NetworkManager.hpp>
#include <globed/core/ServerManager.hpp>
#include <globed/core/FriendListManager.hpp>
#include <globed/core/actions.hpp>
#include <globed/util/Random.hpp>
#include <core/net/NetworkManagerImpl.hpp>
#include <ui/misc/PlayerListCell.hpp>
#include <ui/misc/InputPopup.hpp>
#include <ui/menu/RoomListingPopup.hpp>
#include <ui/menu/CreateRoomPopup.hpp>
#include <ui/menu/TeamManagementPopup.hpp>
#include <ui/settings/SettingsLayer.hpp>
#include <ui/misc/Badges.hpp>

#include <cue/RepeatingBackground.hpp>
#include <UIBuilder.hpp>

using namespace geode::prelude;
using namespace asp::time;

static constexpr CCSize PLAYER_LIST_MENU_SIZE{420.f, 280.f};
static constexpr CCSize PLAYER_LIST_SIZE{PLAYER_LIST_MENU_SIZE.width * 0.8f, 180.f};
static constexpr float CELL_HEIGHT = 27.f;
static constexpr CCSize CELL_SIZE{PLAYER_LIST_SIZE.width, CELL_HEIGHT};

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
        SessionId sessionId
    ) {
        auto ret = new PlayerCell();
        ret->m_sessionId = sessionId;

        if (ret->init(accountId, userId, username, icons, CELL_SIZE)) {
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

protected:
    SessionId m_sessionId;

    bool customSetup() {
        if (m_sessionId.asU64() != 0) {
            // add button to join the session
            Build<CCSprite>::createSpriteName("GJ_playBtn2_001.png")
                .scale(0.31f)
                .intoMenuItem([this] {
                    globed::warpToSession(m_sessionId, false, true);
                })
                .zOrder(10)
                .pos(this->getContentWidth() - 30.f, this->getContentHeight() / 2.f)
                .scaleMult(1.1f)
                .parent(m_rightMenu);

            m_rightMenu->updateLayout();
        }

        return true;
    }
};

// order of buttons in right side menu
namespace RightBtn {
    constexpr int Invisibility = 3;
    constexpr int AdminPanel = 4;
    constexpr int Search = 5;
    constexpr int ClearSearch = 6;
    constexpr int Invite = 7;
    constexpr int Refresh = 100; // dead last
}

namespace LeftBtn {
    constexpr int Disconnect = 100;
    constexpr int Teams = 300;
}

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
                .intoMenuItem([] {
                    // TODO: let the user edit the server
                })
                .store(m_editServerButton)
        )
        .contentSize(CONNECT_MENU_WIDTH, 40.f)
        .updateLayout()
        .parent(m_connectMenu);

    m_connectButton = Build<ButtonSprite>::create("Connect", "bigFont.fnt", "GJ_button_01.png", 0.7f)
        .scale(0.9f)
        .intoMenuItem([this] {
            // TODO: yeah
            auto override = globed::value<std::string>("core.dev.override-central-url");
            std::string url = override ? *override : "tcp://[::1]:53781";

            if (auto err = NetworkManager::get().connectCentral(url).err()) {
                log::error("failed to connect to central server: {}", err);
                return;
            }
        })
        .scaleMult(1.1f)
        .parent(m_connectMenu);

    // connection state label
    m_connStateLabel = Build<CCLabelBMFont>::create("", "bigFont.fnt")
        .scale(0.6f)
        .id("conn-state-lbl")
        .parent(m_connectMenu);

    // button menu
    auto buttonMenu = Build<CCMenu>::create()
        .contentSize(CONNECT_MENU_WIDTH, 40.f)
        .layout(RowLayout::create()->setAutoScale(false))
        .parent(m_connectMenu)
        .collect();

    Build<CCSprite>::create("icon-settings.png"_spr)
        .intoMenuItem([this] {
            this->onSettings();
        })
        .scaleMult(1.1f)
        .parent(buttonMenu);

    // TODO: more buttons here, discord, website, status website, etc.

    buttonMenu->updateLayout();

    // player list menu

    m_playerListMenu = Build<CCNode>::create()
        .id("player-list-menu")
        .contentSize(PLAYER_LIST_MENU_SIZE)
        .pos(winSize / 2.f)
        .anchorPoint(0.5f, 0.5f)
        .parent(this)
        .visible(false);

    Build(CCScale9Sprite::create("GJ_square02.png", {0, 0, 80, 80}))
        .contentSize(PLAYER_LIST_MENU_SIZE)
        .pos(PLAYER_LIST_MENU_SIZE / 2.f)
        .parent(m_playerListMenu);

    m_roomNameButton = Build<CCLabelBMFont>::create("", "goldFont.fnt")
        .scale(0.7f)
        .store(m_roomNameLabel)
        .intoMenuItem([this] {
            this->copyRoomIdToClipboard();
        })
        .pos(PLAYER_LIST_MENU_SIZE.width / 2.f, PLAYER_LIST_MENU_SIZE.height - 18.f)
        .id("room-name-lbl")
        .parent(m_playerListMenu);

    m_playerList = Build<cue::ListNode>::create(PLAYER_LIST_SIZE, ccColor4B{0x33, 0x44, 0x99, 255}, cue::ListBorderStyle::CommentsBlue)
        .anchorPoint(0.5f, 1.f)
        .pos(PLAYER_LIST_MENU_SIZE.width / 2.f, PLAYER_LIST_MENU_SIZE.height - 36.f)
        .parent(m_playerListMenu);

    m_playerList->setJustify(cue::Justify::Center);
    m_playerList->setCellHeight(CELL_HEIGHT);
    m_playerList->setCellColors(
        ccColor4B{0x28, 0x35, 0x77, 255},
        ccColor4B{0x33, 0x44, 0x99, 255}
    );

    m_roomButtonsMenu = Build<CCMenu>::create()
        .id("room-buttons")
        .layout(RowLayout::create()->setAutoScale(false))
        .contentSize(PLAYER_LIST_SIZE.width, 64.f)
        .pos(PLAYER_LIST_MENU_SIZE.width / 2.f, 32.f)
        .parent(m_playerListMenu);

    m_leftSideMenu = Build<CCMenu>::create()
        .id("left-side-menu")
        .layout(ColumnLayout::create()->setAutoScale(false)->setAxisReverse(true)->setAxisAlignment(AxisAlignment::End))
        .contentSize(PLAYER_LIST_MENU_SIZE.width * 0.08f, PLAYER_LIST_MENU_SIZE.height - 12.f)
        .pos(8.f, PLAYER_LIST_MENU_SIZE.height - 6.f)
        .anchorPoint(0.f, 1.f)
        .parent(m_playerListMenu);

    m_rightSideMenu = Build<CCMenu>::create()
        .id("right-side-menu")
        .layout(ColumnLayout::create()->setAutoScale(false)->setAxisReverse(true)->setAxisAlignment(AxisAlignment::End))
        .contentSize(PLAYER_LIST_MENU_SIZE.width * 0.08f, PLAYER_LIST_MENU_SIZE.height - 12.f)
        .pos(PLAYER_LIST_MENU_SIZE - CCSize{8.f, 6.f})
        .anchorPoint(1.f, 1.f)
        .parent(m_playerListMenu);

    // init far menus

    m_farLeftMenu = Build<CCMenu>::create()
        .id("far-left-menu")
        .layout(ColumnLayout::create()->setAutoScale(false)->setAxisReverse(true)->setAxisAlignment(AxisAlignment::Start))
        .contentSize(48.f, 250.f)
        .pos(16.f, 18.f)
        .anchorPoint(0.f, 0.f)
        .parent(this);

    m_farRightMenu = Build<CCMenu>::create()
        .id("far-right-menu")
        .layout(ColumnLayout::create()->setAutoScale(false)->setAxisReverse(true)->setAxisAlignment(AxisAlignment::Start))
        .contentSize(48.f, 250.f)
        .pos(winSize.width - 16.f, 18.f)
        .anchorPoint(1.f, 0.f)
        .parent(this);

    m_roomStateListener = NetworkManagerImpl::get().listen<msg::RoomStateMessage>([this](const auto& msg) {
        if (msg.roomId != m_roomId) {
            this->initNewRoom(msg.roomId, msg.roomName, msg.players, msg.settings);
        } else {
            this->updateRoom(msg.roomName, msg.players, msg.settings);
        }

        return ListenerResult::Continue;
    });


    this->onServerModified();

    this->update(0.f);
    this->scheduleUpdate();

    return true;
}

void GlobedMenuLayer::initNewRoom(uint32_t id, const std::string& name, const std::vector<RoomPlayer>& players, const RoomSettings& settings) {
    m_roomId = id;
    m_roomNameLabel->setString(fmt::format("{} ({})", name, id).c_str());

    this->updateRoom(name, players, settings);
    this->initRoomButtons();
    this->initSideButtons();
}

void GlobedMenuLayer::updateRoom(const std::string& name, const std::vector<RoomPlayer>& players, const RoomSettings& settings) {
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

    // add ourself
    sortedPlayers.push_back(RoomPlayer::createMyself());

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
            player.session
        ));
    }

    m_playerList->setAutoUpdate(true);
    m_playerList->updateLayout();
    m_playerList->setScrollPos(scrollPos);
}

void GlobedMenuLayer::initRoomButtons() {
    m_roomButtonsMenu->removeAllChildren();

    constexpr float BtnScale = 0.77f;

    if (m_roomId == 0) {
        // global room, show buttons to create / join a room

        Build(ButtonSprite::create("Join Room", "bigFont.fnt", "GJ_button_01.png", BtnScale))
            .intoMenuItem([] {
                if (auto p = RoomListingPopup::create()) p->show();
            })
            .scaleMult(1.1f)
            .parent(m_roomButtonsMenu);

        Build(ButtonSprite::create("Create Room", "bigFont.fnt", "GJ_button_01.png", BtnScale))
            .intoMenuItem([] {
                CreateRoomPopup::create()->show();
            })
            .scaleMult(1.1f)
            .parent(m_roomButtonsMenu);
    } else {
        Build(ButtonSprite::create("Leave Room", "bigFont.fnt", "GJ_button_01.png", BtnScale))
            .intoMenuItem([] {
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

    auto makeButton = [this](CCSprite* sprite, std::optional<EditorBaseColor> color, CCNode* parent, int zOrder, const char* id, auto cb) {
        CCSprite* spr;
        if (color) {
            spr = EditorButtonSprite::create(sprite, *color);
        } else {
            spr = sprite;
        }

        Build(spr)
            .with([&](auto btn) { cue::rescaleToMatch(btn, buttonSize); })
            .intoMenuItem([cb = std::move(cb)](auto) {
                cb();
            })
            .zOrder(zOrder)
            .scaleMult(1.1f)
            .id(id)
            .parent(parent);
    };

    /// Left side buttons

    makeButton(
        CCSprite::create("icon-leave-server.png"_spr),
        std::nullopt,
        m_leftSideMenu,
        LeftBtn::Disconnect,
        "btn-disconnect",
        [this] {
            globed::quickPopup("Note", "Are you sure you want to <cr>disconnect</c> from the server?", "Cancel", "Yes", [](auto, bool yes) {
                if (!yes) return;

                if (auto err = NetworkManagerImpl::get().disconnectCentral().err()) {
                    globed::toastError("{}", *err);
                }
            });
        }
    );

    if (RoomManager::get().getSettings().teams) {
        makeButton(
            CCSprite::create("icon-person.png"_spr),
            EditorBaseColor::Cyan,
            m_leftSideMenu,
            LeftBtn::Teams,
            "btn-manage-teams",
            [this] {
                TeamManagementPopup::create()->show();
            }
        );
    }

    /// Right side buttons

    // TODO: filter button

    // mod panel button
    if (NetworkManagerImpl::get().isModerator()) {
        auto badge = globed::createMyBadge();
        if (!badge) {
            badge = globed::createBadge("role-unknown.png");
        }

        if (badge) {
            makeButton(
                badge,
                EditorBaseColor::Aqua,
                m_rightSideMenu,
                RightBtn::AdminPanel,
                "btn-managemod-panel",
                [this] { globed::openModPanel(); }
            );

            badge->setScale(badge->getScale() * 0.85f);
        } else {
            log::error("Failed to create mod panel button, badge not found");
        }
    }

    m_rightSideMenu->updateLayout();
    m_leftSideMenu->updateLayout();
}

void GlobedMenuLayer::initFarSideButtons() {
    auto winSize = CCDirector::get()->getWinSize();
    m_farLeftMenu->removeAllChildren();
    m_farRightMenu->removeAllChildren();

    bool connected = m_state == MenuState::Connected;

    if (connected) {
        Build<CCSprite>::create("icon-settings.png"_spr)
            .intoMenuItem([this] {
                this->onSettings();
            })
            .scaleMult(1.1f)
            .parent(m_farLeftMenu);
    }

    m_farLeftMenu->updateLayout();
    m_farRightMenu->updateLayout();
}

void GlobedMenuLayer::copyRoomIdToClipboard() {
    geode::utils::clipboard::write(fmt::to_string(m_roomId));
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

    m_connStateLabel->limitLabelWidth(CONNECT_MENU_WIDTH, 0.7f, 0.2f);

    this->setMenuState(newState);

    if (newState == MenuState::Connected) {
        if (m_roomId == -1 && (!m_lastRoomUpdate || m_lastRoomUpdate->elapsed() > Duration::fromSecs(1))) {
            // request room state
            m_lastRoomUpdate = Instant::now();
            NetworkManagerImpl::get().sendRoomStateCheck();
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
            m_connStateLabel->setVisible(false);
            m_playerListMenu->setVisible(false);
        } break;

        case MenuState::Connecting: {
            m_connectMenu->setVisible(true);
            m_editServerButton->setEnabled(false);
            m_connectButton->setVisible(false);
            m_connStateLabel->setVisible(true);
            m_playerListMenu->setVisible(false);
        } break;

        case MenuState::Connected: {
            m_connectMenu->setVisible(false);
            m_playerListMenu->setVisible(true);
        } break;
    }

    this->initFarSideButtons();

    // yikes
    m_connectMenuBg->setVisible(false);
    m_connectMenu->updateLayout();
    m_connectMenuBg->setVisible(true);
}

void GlobedMenuLayer::keyBackClicked() {
    auto& rm = RoomManager::get();
    if (!rm.isInFollowerRoom() || rm.isOwner()) {
        // only the owner of a follower room can leave to the main menu
        BaseLayer::keyBackClicked();
    } else {
        // TODO: make this not the case
        globed::alert(
            "Error",
            "You are in a follower room, you <cr>cannot</c> leave to the main menu. Only the room owner can choose which levels to play."
        );
    }
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
