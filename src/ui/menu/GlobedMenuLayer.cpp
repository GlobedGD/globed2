#include "GlobedMenuLayer.hpp"
#include <globed/core/SettingsManager.hpp>
#include <globed/core/net/NetworkManager.hpp>
#include <core/net/NetworkManagerImpl.hpp>
#include <ui/misc/PlayerListCell.hpp>

#include <cue/RepeatingBackground.hpp>
#include <UIBuilder.hpp>

using namespace geode::prelude;
using namespace asp::time;

static constexpr float CELL_HEIGHT = 27.f;
static constexpr CCSize PLAYER_LIST_MENU_SIZE{420.f, 280.f};
static constexpr CCSize PLAYER_LIST_SIZE{PLAYER_LIST_MENU_SIZE.width * 0.8f, 180.f};

namespace globed {

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
    m_connectMenu = Build<CCMenu>::create()
        .id("connect-menu")
        .layout(ColumnLayout::create()->setAxisReverse(true)->setAutoScale(false))
        .pos(winSize.width / 2.f, winSize.height * 0.7f)
        .contentSize(0.f, 80.f)
        .anchorPoint(0.5f, 0.5f)
        .parent(this);

    auto connectMenuLayout = RowLayout::create()->setAutoScale(false);
    connectMenuLayout->ignoreInvisibleChildren(true);
    Build<CCMenu>::create()
        .id("server-field")
        .layout(connectMenuLayout)
        .child(
            // TODO: placeholder text
            Build<CCLabelBMFont>::create("Server Name", "goldFont.fnt")
                .limitLabelWidth(180.f, 0.7f, 0.1f)
        )
        .child(
            Build<CCSprite>::create("pencil.png"_spr)
                .scale(0.7f)
                .intoMenuItem([] {
                    // TODO: let the user edit the server
                })
                .store(m_editServerButton)
        )
        .contentSize(220.f, 0.f)
        .updateLayout()
        .parent(m_connectMenu);

    m_connectButton = Build<ButtonSprite>::create("Connect", "bigFont.fnt", "GJ_button_01.png", 0.7f)
        .intoMenuItem([this] {
            // TODO: yeah
            auto override = globed::value<std::string>("core.dev.override-central-url");
            std::string url = override ? *override : "tcp://[::1]:53781";

            if (auto err = NetworkManager::get().connectCentral(url).err()) {
                log::error("failed to connect to central server: {}", err);
                return;
            }
        })
        .parent(m_connectMenu);

    // connection state label
    m_connStateLabel = Build<CCLabelBMFont>::create("", "bigFont.fnt")
        .id("conn-state-lbl")
        .parent(m_connectMenu);

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

    m_roomStateListener = NetworkManagerImpl::get().listen<msg::RoomStateMessage>([this](const auto& msg) {
        if (msg.roomId != m_roomId) {
            this->initNewRoom(msg.roomId, msg.roomName, msg.players);
        }

        return ListenerResult::Continue;
    });

    this->update(0.f);
    this->scheduleUpdate();

    return true;
}

void GlobedMenuLayer::initNewRoom(uint32_t id, const std::string& name, const std::vector<RoomPlayer>& players) {
    m_roomId = id;
    m_roomNameLabel->setString(fmt::format("{} ({})", name, id).c_str());

    m_playerList->setAutoUpdate(false);
    m_playerList->clear();

    CCSize cellSize{PLAYER_LIST_SIZE.width, CELL_HEIGHT};

    for (auto& player : players) {
        m_playerList->addCell(PlayerListCell::create(
            player.accountData.accountId,
            player.accountData.userId,
            player.accountData.username,
            cue::Icons {
                .type = IconType::Cube,
                .id = player.cube,
                .color1 = player.color1,
                .color2 = player.color2,
                .glowColor = NO_GLOW,
            },
            cellSize
        ));
    }

    // add ourself
    auto gam = cachedSingleton<GJAccountManager>();
    m_playerList->addCell(PlayerListCell::createMyself(cellSize));

    // TODO: sort

    m_playerList->setAutoUpdate(true);
    m_playerList->updateLayout();

    this->initRoomButtons();
}

void GlobedMenuLayer::initRoomButtons() {
    m_roomButtonsMenu->removeAllChildren();

    constexpr float BtnScale = 0.77f;

    if (m_roomId == 0) {
        // global room, show buttons to create / join a room

        Build(ButtonSprite::create("Join Room", "bigFont.fnt", "GJ_button_01.png", BtnScale))
            .intoMenuItem([] {
                NetworkManagerImpl::get().sendJoinRoom(123456);
            })
            .scaleMult(1.1f)
            .parent(m_roomButtonsMenu);

        Build(ButtonSprite::create("Create Room", "bigFont.fnt", "GJ_button_01.png", BtnScale))
            .intoMenuItem([] {
                // TODO: show a cool ass popup with room creation options
                NetworkManagerImpl::get().sendCreateRoom("Cool room", 0, RoomSettings{});
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

void GlobedMenuLayer::copyRoomIdToClipboard() {
    geode::utils::clipboard::write(fmt::to_string(m_roomId));
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
            m_connStateLabel->setString("Establishing connection...");
        } break;

        case ConnectionState::Connected: {
            newState = MenuState::Connected;
        } break;

        case ConnectionState::Closing: {
            newState = MenuState::Connecting;
            m_connStateLabel->setString("Closing connection...");
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

    this->setMenuState(newState);

    if (newState == MenuState::Connected) {
        if (m_roomId == -1 && (!m_lastRoomUpdate || m_lastRoomUpdate->elapsed() > Duration::fromSecs(1))) {
            // request room state
            m_lastRoomUpdate = Instant::now();
            NetworkManagerImpl::get().sendRoomStateCheck();
        }
    }
}

void GlobedMenuLayer::setMenuState(MenuState state) {
    switch (state) {
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

    m_connectMenu->updateLayout();
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
