#include "CreateRoomPopup.hpp"
#include <globed/core/PopupManager.hpp>
#include <globed/core/ValueManager.hpp>
#include <core/net/NetworkManagerImpl.hpp>
#include <globed/util/gd.hpp>
#include <ui/misc/LoadingPopup.hpp>

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

const CCSize CreateRoomPopup::POPUP_SIZE = {420.f, 240.f};
static constexpr int TAG_PRIVATE = 1021;
static constexpr int TAG_CLOSED_INVITES = 1022;
static constexpr int TAG_COLLISION = 1023;
static constexpr int TAG_TWO_PLAYER= 1024;
static constexpr int TAG_DEATHLINK = 1025;
static constexpr int TAG_TEAMS = 1026;

bool CreateRoomPopup::setup() {
    this->setID("create-room-popup"_spr);
    this->setTitle("Create Room", "goldFont.fnt", 1.0f);

    m_inputsWrapper = Build<CCNode>::create()
        .id("inputs-wrapper")
        .anchorPoint(0.f, 0.5f)
        .pos(this->centerLeft() + CCPoint{15.f, 10.f})
        .layout(ColumnLayout::create()->setAutoScale(false)->setGap(3.f))
        .contentSize(0.f, POPUP_SIZE.height * 0.6f)
        .parent(m_mainLayer)
        .collect();

    auto smallInputsWrapper = Build<CCNode>::create()
        .id("small-wrapper")
        .layout(RowLayout::create()->setAutoScale(false))
        .parent(m_inputsWrapper)
        .collect();

    // room name
    auto roomNameWrapper = Build<CCNode>::create()
        .id("name-wrapper")
        .layout(ColumnLayout::create()->setAxisReverse(true)->setAutoScale(false))
        .contentSize(0.f, 55.f)
        .intoNewChild(CCMenu::create()) // label wrapper
        .contentSize(100.f, 20.f)
        .layout(RowLayout::create()->setGap(3.f)->setAutoScale(false))
        .child(
            Build<CCLabelBMFont>::create("Room Name", "bigFont.fnt")
                .scale(0.5f)
        )
        .child(
            Build<CCSprite>::createSpriteName("GJ_infoIcon_001.png")
                .scale(0.5f)
                .intoMenuItem([this] {
                    this->showRoomNameWarnPopup();
                })
        )
        .updateLayout()
        .intoParent() // into name-wrapper
        .parent(m_inputsWrapper)
        .intoNewChild(TextInput::create(POPUP_SIZE.width * 0.515f, "", "chatFont.fnt"))
        .with([&](TextInput* input) {
            input->setCommonFilter(CommonFilter::Any);
            input->setMaxCharCount(32);
        })
        .store(m_nameInput)
        .intoParent()
        .updateLayout()
        .collect();

    // password
    Build<CCNode>::create()
        .id("pw-wrapper")
        .layout(ColumnLayout::create()->setAxisReverse(true)->setAutoScale(false))
        .contentSize(0.f, 50.f)
        .intoNewChild(CCLabelBMFont::create("Password", "bigFont.fnt"))
        .scale(0.35f)
        .intoParent()
        .parent(smallInputsWrapper)
        .intoNewChild(TextInput::create(POPUP_SIZE.width * 0.25f, "", "chatFont.fnt"))
        .with([&](TextInput* input) {
            input->setCommonFilter(CommonFilter::Uint);
            input->setMaxCharCount(16);
        })
        .store(m_passcodeInput)
        .intoParent()
        .updateLayout();

    // player limit
    Build<CCNode>::create()
        .id("limit-wrapper")
        .layout(ColumnLayout::create()->setAxisReverse(true)->setAutoScale(false))
        .contentSize(0.f, 50.f)
        .intoNewChild(CCLabelBMFont::create("Player limit", "bigFont.fnt"))
        .scale(0.35f)
        .intoParent()
        .parent(smallInputsWrapper)
        .intoNewChild(TextInput::create(POPUP_SIZE.width * 0.25f, "", "chatFont.fnt"))
        .with([&](TextInput* input) {
            input->setCommonFilter(CommonFilter::Uint);
            input->setMaxCharCount(6);
        })
        .store(m_playerLimitInput)
        .intoParent()
        .updateLayout();

    // classic / follower room
    m_followerWrapper = Build<CCMenu>::create()
        .id("follower-wrapper")
        .zOrder(-1)
        .layout(RowLayout::create()->setAutoScale(false)->setGap(5.f))
        .contentSize(POPUP_SIZE.width * 0.515f, 0.f)
        .parent(m_inputsWrapper);
    m_followerWrapper->setLayoutOptions(AxisLayoutOptions::create()->setNextGap(8.f));

    this->setFollowerMode(false);

    // cancel/create buttons
    Build<CCMenu>::create()
        .id("btn-wrapper")
        .layout(RowLayout::create()->setAutoScale(false))
        .parent(m_mainLayer)
        .child(
            Build<ButtonSprite>::create("Cancel", "goldFont.fnt", "GJ_button_05.png", 0.8f)
                .intoMenuItem([this](auto) {
                    this->onClose(nullptr);
                })
                .scaleMult(1.1f)
                .collect()
        )
        .child(
            Build<ButtonSprite>::create("Create", "goldFont.fnt", "GJ_button_04.png", 0.8f)
                .intoMenuItem([this](auto) {
                    std::string roomName = m_nameInput->getString();
                    if (roomName.empty()) {
                        roomName = fmt::format("{}'s room", GJAccountManager::get()->m_username);
                    }

                    geode::utils::string::trimIP(roomName);

                    // parse as a 32-bit int but cap at 9999
                    // this is so that if a user inputs a number like 99999 (doesnt fit),
                    // instead of making it 0, it makes it 9999

                    uint32_t playerCount = geode::utils::numFromString<uint32_t>(m_playerLimitInput->getString()).unwrapOr(0);
                    m_settings.playerLimit = std::min<uint32_t>(playerCount, 9999);

                    if (m_settings.playerLimit == 1488) {
                        globed::alert("Error", "Please choose a <cr>different</c> player limit number.");
                        return;
                    }

                    auto passcodeStr = m_passcodeInput->getString();
                    uint32_t passcode = 0;

                    if (!passcodeStr.empty()) {
                        auto res = geode::utils::numFromString<uint32_t>(m_passcodeInput->getString());
                        if (!res) {
                            globed::alert("Error", "Invalid passcode, must consist of 1 to 10 digits.");
                            return;
                        }

                        passcode = *res;
                    }

                    // pick the preferred server
                    if (auto srv = NetworkManagerImpl::get().getPreferredServer()) {
                        m_settings.serverId = *srv;
                    } else {
                        globed::alert("Error", "No game servers are available to host this room. This is a <cr>server issue</c>, please <cj>reconnect to the server and try again</c>.");
                        return;
                    }

                    m_settings.fasterReset = gameVariable(GameVariable::FastRespawn);

                    this->waitForResponse();

                    // send the packet after setting up listeners, avoiding race condition if the server is on localhost
                    NetworkManagerImpl::get().sendCreateRoom(roomName, passcode, m_settings);
                })
                .scaleMult(1.1f)
                .collect()
        )
        .pos(this->fromBottom(25.f))
        .updateLayout();

    smallInputsWrapper->setContentWidth(roomNameWrapper->getScaledContentWidth());
    smallInputsWrapper->updateLayout();
    m_inputsWrapper->updateLayout();

    // safe mode button
    Build<CCSprite>::create("white-period.png"_spr)
        .color(ccGREEN)
        .intoMenuItem([this] {
            this->showSafeModePopup(false);
        })
        .store(m_safeModeBtn)
        .parent(m_buttonMenu)
        .pos(this->fromTopRight(16.f, 16.f));

    // list of settings
    const float gap = 3.f;
    auto* settingsList = Build<CCNode>::create()
        .id("settings")
        .anchorPoint(1.f, 0.5f)
        .pos(this->centerRight() - CCPoint{15.f, 0.f})
        .layout(ColumnLayout::create()->setAxisReverse(true)->setGap(gap))
        .contentSize(0.f, POPUP_SIZE.height * 0.65f)
        .parent(m_mainLayer)
        .collect();

    auto settings = std::to_array<std::tuple<const char*, std::string_view, int, CCMenuItemToggler**>>({
        {"Hidden Room", "hidden-room"_spr, TAG_PRIVATE, nullptr},
        {"Closed Invites", "closed-invites"_spr, TAG_CLOSED_INVITES, nullptr},
        {"Collision", "collision"_spr, TAG_COLLISION, nullptr},
        {"2-Player Mode", "2-player-mode"_spr, TAG_TWO_PLAYER, &m_twoPlayerBtn},
        {"Death Link", "deathlink"_spr, TAG_DEATHLINK, &m_deathlinkBtn},
        {"Teams", "teams"_spr, TAG_TEAMS, nullptr},
    });

    float totalHeight = 0.f;

    for (const auto& entry : settings) {
        const float height = 15.5f;
        const float width = 110.5f;

        if (totalHeight != 0.f) {
            // gap between settings
            totalHeight += gap;
        }

        totalHeight += height;

        CCMenuItemToggler* toggler;
        Build(CCMenuItemToggler::create(
            Build<CCSprite>::createSpriteName("GJ_checkOff_001.png").scale(0.5f),
            Build<CCSprite>::createSpriteName("GJ_checkOn_001.png").scale(0.5f),
            this, menu_selector(CreateRoomPopup::onCheckboxToggled)
        ))
            .pos(width - 11.f, height / 2.f)
            .id(std::string(std::get<1>(entry)))
            .tag(std::get<2>(entry))
            .store(toggler)
            .intoNewParent(CCMenu::create())
            .contentSize(width, height)
            .parent(settingsList)
            .intoNewChild(CCLabelBMFont::create(std::get<0>(entry), "bigFont.fnt"))
            .limitLabelWidth(width - 30.f, 0.35f, 0.1f)
            .anchorPoint(0.f, 0.5f)
            .pos(3.f, height / 2.f)
            .intoParent()
            .updateLayout();

        auto outptr = std::get<3>(entry);
        if (outptr) {
            *outptr = toggler;
        }
    }

    settingsList->setContentHeight(totalHeight + 5.f);
    settingsList->updateLayout();

    // add bg
    float sizeScale = 3.f;
    Build<CCScale9Sprite>::create("square02_001.png")
        .opacity(67)
        .zOrder(-1)
        .contentSize(settingsList->getScaledContentSize() * sizeScale + CCPoint{8.f, 8.f})
        .scaleX(1.f / sizeScale)
        .scaleY(1.f / sizeScale)
        .parent(settingsList)
        .anchorPoint(0.5f, 0.5f)
        .pos(settingsList->getScaledContentSize() / 2.f);

    return true;
}

void CreateRoomPopup::setFollowerMode(bool follower) {
    m_followerWrapper->removeAllChildren();

    m_settings.isFollower = follower;

    Build<ButtonSprite>::create("Classic", "bigFont.fnt", follower ? "GJ_button_05.png" : "GJ_button_01.png", 0.8f)
        .scale(0.8f)
        .intoMenuItem([this](auto) {
            this->setFollowerMode(false);
        })
        .scaleMult(1.1f)
        .parent(m_followerWrapper);

    Build<ButtonSprite>::create("Follower", "bigFont.fnt", follower ? "GJ_button_01.png" : "GJ_button_05.png", 0.8f)
        .scale(0.8f)
        .intoMenuItem([this](auto) {
            this->setFollowerMode(true);
        })
        .scaleMult(1.1f)
        .parent(m_followerWrapper);

    auto btn = Build<CCSprite>::createSpriteName("GJ_infoIcon_001.png")
        .scale(0.5f)
        .intoMenuItem([this](auto) {
            globed::alert(
                "Info",
                "Follower rooms only allow the host to join levels, and the rest of the players will be instantly warped to the level the host joins."
            );
        })
        .parent(m_followerWrapper)
        .collect();

    m_followerWrapper->updateLayout();

    btn->setPositionY(btn->getPositionY() + 14.f);
}

void CreateRoomPopup::onCheckboxToggled(cocos2d::CCObject* p) {
    auto* btn = static_cast<CCMenuItemToggler*>(p);
    bool state = !btn->isOn();

    bool isSafeMode = false;

    switch (p->getTag()) {
        case TAG_TWO_PLAYER: isSafeMode = true; m_settings.twoPlayerMode = state; break;
        case TAG_COLLISION: isSafeMode = true; m_settings.collision = state; break;
        case TAG_CLOSED_INVITES: m_settings.privateInvites = state; break;
        case TAG_PRIVATE: m_settings.hidden = state; break;
        case TAG_DEATHLINK: m_settings.deathlink = state; break;
        case TAG_TEAMS: m_settings.teams = state; break;
    }

    if (isSafeMode && state && !globed::value<bool>("core.flags.seen-room-safe-mode-notice")) {
        globed::setValue("core.flags.seen-room-safe-mode-notice", true);
        this->showSafeModePopup(true);
    }

    if (p->getTag() == TAG_DEATHLINK && m_settings.deathlink && m_settings.twoPlayerMode) {
        m_settings.twoPlayerMode = false;
        m_twoPlayerBtn->toggle(false);
    } else if (p->getTag() == TAG_TWO_PLAYER && m_settings.twoPlayerMode && m_settings.deathlink) {
        m_settings.deathlink = false;
        m_deathlinkBtn->toggle(false);
    }

    bool enableSafeModeDot = (m_settings.twoPlayerMode || m_settings.collision);

    m_safeModeBtn->getChildByType<CCSprite>(0)->setColor(enableSafeModeDot ? ccORANGE : ccGREEN);
}

void CreateRoomPopup::showSafeModePopup(bool firstTime) {
    auto getSafeModeString = [&]() -> std::string {
        std::string mods;

        if (m_settings.twoPlayerMode) {
            mods += "2-Player Mode";
        }

        if (m_settings.collision) {
            if (!mods.empty()) {
                mods += ", ";
            }

            mods += "Collision";
        }

        if (mods.find(", ") != std::string::npos) {
            return fmt::format("<cy>Safe mode</c> is <cr>enabled</c> due to the following settings: <cy>{}</c>.\n\nYou won't be able to make progress on levels while these settings are enabled.", mods);
        } else {
            return fmt::format("<cy>Safe mode</c> is <cr>enabled</c> due to the following setting: <cy>{}</c>.\n\nYou won't be able to make progress on levels while this setting is enabled.", mods);
        }
    };

    if (!m_settings.twoPlayerMode && !m_settings.collision) {
        globed::alert("Safe Mode", "<cy>Safe Mode</c> is <cg>not enabled</c> with these room settings. You are able to make progress on levels while in this room.");
        return;
    }

    globed::alert(
        "Safe Mode",
        firstTime
            ? "This setting enables <cy>safe mode</c>, which means you won't be able to make progress on levels while in this room."
            : getSafeModeString()
    );
}

void CreateRoomPopup::showRoomNameWarnPopup() {
    globed::alert(
        "Note",

        "Room names should be clear and appropriate. "
        "Creating a room with <cy>advertisements</c> or <cr>profanity</c> in its name may lead to a <cy>closure of the room</c>, "
        "or in some cases a <cr>ban</c>."
    );
}

void CreateRoomPopup::waitForResponse() {
    // wait for either a room state mesage or a room create failed message

    m_loadingPopup = LoadingPopup::create();
    m_loadingPopup->setTitle("Creating Room...");
    m_loadingPopup->setClosable(true);
    m_loadingPopup->show();

    m_successListener = NetworkManagerImpl::get().listen<msg::RoomStateMessage>([this](const auto& msg) {
        // small sanity check to make sure it is actually the response we need
        if (msg.roomOwner == cachedSingleton<GJAccountManager>()->m_accountID) {
            this->stopWaiting(std::nullopt);
        }

        return ListenerResult::Continue;
    });
    m_successListener.value()->setPriority(-100);

    m_failListener = NetworkManagerImpl::get().listen<msg::RoomCreateFailedMessage>([this](const auto& msg) {
        using enum msg::RoomCreateFailedReason;
        std::string reason;

        switch (msg.reason) {
            case InvalidName: reason = "Invalid room name"; break;
            case InvalidPasscode: reason = "Invalid passcode"; break;
            case InvalidSettings: reason = "Invalid room settings"; break;
            case InvalidServer: reason = "Invalid server chosen, this server is unavailable. Please choose a different server or try again later"; break;
            case ServerDown: reason = "The server is currently down, please try again later"; break;
            case InappropriateName: reason = "Inappropriate room name, please choose a different name. Bypassing the filter may result in a <cr>ban</c>."; break;
            default: reason = "Unknown reason"; break;
        }

        this->stopWaiting(reason);

        return ListenerResult::Stop;
    });
}

void CreateRoomPopup::stopWaiting(std::optional<std::string> failReason) {
    m_loadingPopup->forceClose();
    m_loadingPopup = nullptr;
    m_failListener.reset();
    m_successListener.reset();

    if (failReason) {
        globed::alertFormat("Error", "Failed to create room: <cy>{}</c>", *failReason);
    } else {
        this->onClose(nullptr);
    }
}

}