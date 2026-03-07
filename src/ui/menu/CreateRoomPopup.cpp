#include "CreateRoomPopup.hpp"
#include <globed/core/PopupManager.hpp>
#include <globed/core/ValueManager.hpp>
#include <globed/core/ServerManager.hpp>
#include <core/net/NetworkManagerImpl.hpp>
#include <globed/util/gd.hpp>
#include <ui/misc/LoadingPopup.hpp>
#include <ui/menu/RoomSettingsPopup.hpp>

#include <ui/Core.hpp>

using namespace geode::prelude;
using namespace asp::time;

namespace globed {

static std::string defaultName() {
    return fmt::format("{}'s room", GJAccountManager::get()->m_username);
}

bool CreateRoomPopup::init() {
    if (!BasePopup::init(240.f, 190.f)) return false;

    this->setID("create-room-popup"_spr);
    this->setTitle("Create Room", "goldFont.fnt", 0.7f, 15.f);

    bool canName = NetworkManagerImpl::get().getUserPermissions().canNameRooms;

    m_inputsWrapper = Build<CCNode>::create()
        .id("inputs-wrapper")
        .pos(this->fromCenter(0.f, 10.f))
        .anchorPoint(0.5f, 0.5f)
        .layout(ColumnLayout::create()->setAutoScale(false)->setGap(3.f))
        .contentSize(0.f, m_size.height * 0.6f)
        .parent(m_mainLayer)
        .collect();

    auto smallInputsWrapper = Build<CCNode>::create()
        .id("small-wrapper")
        .layout(RowLayout::create()->setAutoScale(false))
        .parent(m_inputsWrapper)
        .collect();

    float roomNameWidth = m_size.width * 0.8f;
    float smallInputWidth = (roomNameWidth - 5.f) / 2.f;

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
                .intoMenuItem([this, canName] {
                    this->showRoomNameWarnPopup(canName);
                })
        )
        .updateLayout()
        .intoParent() // into name-wrapper
        .parent(m_inputsWrapper)
        .intoNewChild(TextInput::create(roomNameWidth, "", "chatFont.fnt"))
        .with([&](TextInput* input) {
            gd::string dname(defaultName());

            input->setCommonFilter(CommonFilter::Any);
            input->setMaxCharCount(32);
            input->setPlaceholder(dname);

            if (!canName) {
                input->setString(dname);
                input->setEnabled(false);
            }
        })
        .store(m_nameInput)
        .intoParent()
        .updateLayout()
        .collect();

    bool hidePass = globed::setting<bool>("core.streamer-mode");

    // password
    Build<CCNode>::create()
        .id("pw-wrapper")
        .layout(ColumnLayout::create()->setAxisReverse(true)->setAutoScale(false))
        .contentSize(0.f, 50.f)
        .intoNewChild(CCLabelBMFont::create("Passcode", "bigFont.fnt"))
        .scale(0.35f)
        .intoParent()
        .parent(smallInputsWrapper)
        .intoNewChild(TextInput::create(smallInputWidth, "", hidePass ? "bigFont.fnt" : "chatFont.fnt"))
        .with([&](TextInput* input) {
            input->setCommonFilter(CommonFilter::Uint);
            input->setMaxCharCount(11);
            input->setPasswordMode(hidePass);
            input->setPlaceholder("(none)");
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
        .intoNewChild(TextInput::create(smallInputWidth, "", "chatFont.fnt"))
        .with([&](TextInput* input) {
            input->setCommonFilter(CommonFilter::Uint);
            input->setMaxCharCount(6);
            input->setPlaceholder("Unlimited");
        })
        .store(m_playerLimitInput)
        .intoParent()
        .updateLayout();

    // cancel/create and settings buttons
    auto buttonsCont = Build<RowContainer>::create()
        .id("btn-wrapper")
        .contentSize(0.f, 31.5f)
        .parent(m_mainLayer)
        .pos(this->fromBottom(25.f))
        .collect();
    buttonsCont->layout()->setCrossAxisScaling(AxisScaling::ScaleDown);

    Build<ButtonSprite>::create("Cancel", "goldFont.fnt", "GJ_button_05.png", 0.8f)
        .intoMenuItem([this](auto) {
            this->onClose(nullptr);
        })
        .scaleMult(1.1f)
        .parent(buttonsCont);

    Build<ButtonSprite>::create("Create", "goldFont.fnt", "GJ_button_04.png", 0.8f)
        .intoMenuItem([this](auto) {
            this->onCreate();
        })
        .scaleMult(1.1f)
        .parent(buttonsCont);

    // Button to open room settings
    auto rsButton = Build<CCSprite>::create("settings01.png"_spr)
        .scale(0.8f)
        .intoMenuItem([this] {
            auto popup = RoomSettingsPopup::create(m_settings);
            popup->setCallback([this](RoomSettings settings) {
                m_settings = settings;
            });
            popup->show();
        })
        .scaleMult(1.15f)
        .parent(buttonsCont)
        .collect();

    buttonsCont->updateLayout();
    rsButton->setPositionY(rsButton->getPositionY() - 1.f);

    smallInputsWrapper->setContentWidth(roomNameWrapper->getScaledContentWidth());
    smallInputsWrapper->updateLayout();
    m_inputsWrapper->updateLayout();

    return true;
}

void CreateRoomPopup::showRoomNameWarnPopup(bool canName) {
    if (!canName) {
        auto msg = ServerManager::get().isOfficialServerActive()
            ? "Room names are currently <cy>disabled</c> for regular users due to <cr>abuse</c>. "
            "You can gain the ability to name rooms by <cp>supporting Globed</c>, otherwise "
            "your room will be named automatically."

            : "Room names are <cy>disabled</c> on this server for your account.";

        globed::alert("Info", msg);

        return;
    }

    globed::alert(
        "Note",

        "Room names should be clear and appropriate. "
        "Creating a room with <cy>advertisements</c> or <cr>profanity</c> in its name may lead to a <cy>closure of the room</c>, "
        "or in some cases a <cr>ban</c>."
    );
}

void CreateRoomPopup::onCreate() {
    std::string roomName = m_nameInput->getString();
    if (roomName.empty() || roomName.size() > 32) {
        roomName = defaultName();
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
        log::debug("using server ID: {}", *srv);
        m_settings.serverId = *srv;
    } else {
        globed::alert("Error", "No game servers are available to host this room. This is a <cr>server issue</c>, please <cj>reconnect to the server and try again</c>.");
        return;
    }

    m_settings.fasterReset = gameVariable(GameVariable::FastRespawn);

    this->waitForResponse();

    // send the packet after setting up listeners, avoiding race condition if the server is on localhost
    NetworkManagerImpl::get().sendCreateRoom(roomName, passcode, m_settings);
}

void CreateRoomPopup::waitForResponse() {
    // wait for either a room state mesage or a room create failed message

    m_loadingPopup = LoadingPopup::create();
    m_loadingPopup->setTitle("Creating Room...");
    m_loadingPopup->setClosable(true);
    m_loadingPopup->show();

    auto& nm = NetworkManagerImpl::get();

    m_successListener = nm.listen<msg::RoomStateMessage>([this](const auto& msg) {
        // small sanity check to make sure it is actually the response we need
        if (msg.roomOwner == singleton<GJAccountManager>()->m_accountID) {
            this->stopWaiting(std::nullopt);
        }
    }, -100);

    m_failListener = nm.listen<msg::RoomCreateFailedMessage>([this](const auto& msg) {
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

    m_bannedListener = nm.listen<msg::RoomBannedMessage>([this](const auto& msg) {
        this->stopWaiting(fmt::format(
            "You are banned from creating rooms{}{}. Reason: {}",
            msg.expiresAt == 0 ? " " : " until ",
            msg.expiresAt == 0 ? std::string{"forever"} : SystemTime::fromUnix(msg.expiresAt).toString(),
            msg.reason.empty() ? "(no reason given)" : msg.reason
        ));
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

CreateRoomPopup* CreateRoomPopup::create() {
    auto ret = new CreateRoomPopup();
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

}
