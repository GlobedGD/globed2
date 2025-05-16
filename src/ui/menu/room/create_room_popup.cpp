#include "create_room_popup.hpp"

#include <Geode/ui/TextInput.hpp>

#include "room_layer.hpp"
#include <data/packets/client/room.hpp>
#include <net/manager.hpp>
#include <managers/settings.hpp>
#include <managers/popup.hpp>
#include <util/format.hpp>
#include <util/math.hpp>
#include <util/ui.hpp>
#include <util/gd.hpp>

using namespace geode::prelude;

constexpr int TAG_PRIVATE = 1021;
constexpr int TAG_OPEN_INV = 1022;
constexpr int TAG_COLLISION = 1023;
constexpr int TAG_2P = 1024;
constexpr int TAG_DEATHLINK = 1025;

bool CreateRoomPopup::setup(RoomLayer* parent) {
    this->setID("CreateRoomPopup"_spr);

    this->setTitle("Create Room", "goldFont.fnt", 1.0f);

    auto rlayout = util::ui::getPopupLayoutAnchored(m_size);

    auto inputsWrapper = Build<CCNode>::create()
        .id("inputs-wrapper")
        .anchorPoint(0.f, 0.5f)
        .pos(rlayout.centerLeft + CCPoint{15.f, 10.f})
        .layout(ColumnLayout::create()->setAutoScale(false)->setGap(3.f))
        .contentSize(0.f, POPUP_HEIGHT * 0.6f)
        .parent(m_mainLayer)
        .collect();

    auto smallInputsWrapper = Build<CCNode>::create()
        .id("small-wrapper")
        .layout(RowLayout::create()->setAutoScale(false))
        .parent(inputsWrapper)
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
        .parent(inputsWrapper)
        .intoNewChild(TextInput::create(POPUP_WIDTH * 0.515f, "", "chatFont.fnt"))
        .with([&](TextInput* input) {
            input->setCommonFilter(CommonFilter::Any);
            input->setMaxCharCount(32);
        })
        .store(roomNameInput)
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
        .intoNewChild(TextInput::create(POPUP_WIDTH * 0.25f, "", "chatFont.fnt"))
        .with([&](TextInput* input) {
            input->setFilter(std::string(util::misc::STRING_ALPHANUMERIC));
            input->setMaxCharCount(16);
        })
        .store(passwordInput)
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
        .intoNewChild(TextInput::create(POPUP_WIDTH * 0.25f, "", "chatFont.fnt"))
        .with([&](TextInput* input) {
            input->setFilter(std::string(util::misc::STRING_DIGITS));
            input->setMaxCharCount(6);
        })
        .store(playerLimitInput)
        .intoParent()
        .updateLayout();

    // cancel/create buttons
    Build<CCMenu>::create()
        .id("btn-wrapper")
        .layout(RowLayout::create()->setAutoScale(false))
        .parent(m_mainLayer)
        .child(
            Build<ButtonSprite>::create("Cancel", "goldFont.fnt", "GJ_button_05.png", 0.8f)
                .intoMenuItem([this, parent](auto) {
                    this->onClose(nullptr);
                })
                .collect()
        )
        .child(
            Build<ButtonSprite>::create("Create", "goldFont.fnt", "GJ_button_04.png", 0.8f)
                .intoMenuItem([this, parent](auto) {
                    std::string roomName = roomNameInput->getString();
                    if (roomName.empty()) {
                        roomName = fmt::format("{}'s room", GJAccountManager::get()->m_username);
                    }

                    roomName = util::format::trim(roomName);

                    // parse as a 32-bit int but cap at 9999
                    // this is so that if a user inputs a number like 99999 (doesnt fit),
                    // instead of making it 0, it makes it 9999

                    uint32_t playerCount = util::format::parse<uint32_t>(playerLimitInput->getString()).value_or(0);
                    playerCount = util::math::min(playerCount, 9999);

                    if (playerCount == 1488) {
                        PopupManager::get().alert("Error", "Please choose a different player count number.").showInstant();
                        return;
                    }

                    NetworkManager::get().send(CreateRoomPacket::create(roomName, passwordInput->getString(), RoomSettings {
                        .flags = settingFlags,
                        .playerLimit = static_cast<uint16_t>(playerCount),
                        .fasterReset = util::gd::variable(util::gd::GameVariable::FastRespawn)
                    }));

                    parent->startLoading();
                    this->onClose(nullptr);
                })
                .collect()
        )
        .pos(rlayout.centerBottom + CCPoint{0.f, 25.f})
        .updateLayout();

    smallInputsWrapper->setContentWidth(roomNameWrapper->getScaledContentWidth());
    smallInputsWrapper->updateLayout();
    inputsWrapper->updateLayout();

    // safe mode button
    Build<CCSprite>::createSpriteName("white-period.png"_spr)
        .color(ccGREEN)
        .intoMenuItem([this] {
            this->showSafeModePopup(false);
        })
        .store(safeModeBtn)
        .parent(m_buttonMenu)
        .pos(rlayout.fromTopRight(16.f, 16.f))
        ;

    // list of settings
    const float gap = 3.f;
    auto* settingsList = Build<CCNode>::create()
        .id("settings")
        .anchorPoint(1.f, 0.5f)
        .pos(rlayout.centerRight - CCPoint{15.f, 0.f})
        .layout(ColumnLayout::create()->setAxisReverse(true)->setGap(gap))
        .contentSize(0.f, POPUP_HEIGHT * 0.65f)
        .parent(m_mainLayer)
        .collect()
        ;

    constexpr auto settings = std::to_array<std::tuple<const char*, std::string_view, int>>({
        {"Hidden Room", "hidden-room"_spr, TAG_PRIVATE},
        {"Open Invites", "open-invites"_spr, TAG_OPEN_INV},
        {"Collision", "collision"_spr, TAG_COLLISION},
        {"2-Player Mode", "2-player-mode"_spr, TAG_2P},
        {"Death Link", "deathlink"_spr, TAG_DEATHLINK}
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

        Build(CCMenuItemToggler::create(
            Build<CCSprite>::createSpriteName("GJ_checkOff_001.png").scale(0.5f),
            Build<CCSprite>::createSpriteName("GJ_checkOn_001.png").scale(0.5f),
            this, menu_selector(CreateRoomPopup::onCheckboxToggled)
        ))
            .pos(width - 11.f, height / 2.f)
            .id(std::string(std::get<1>(entry)))
            .tag(std::get<2>(entry))
            .intoNewParent(CCMenu::create())
            .contentSize(width, height)
            .parent(settingsList)
            .intoNewChild(CCLabelBMFont::create(std::get<0>(entry), "bigFont.fnt"))
            .limitLabelWidth(width - 30.f, 0.35f, 0.1f)
            .anchorPoint(0.f, 0.5f)
            .pos(3.f, height / 2.f)
            .intoParent()
            .updateLayout();
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

void CreateRoomPopup::onCheckboxToggled(cocos2d::CCObject* p) {
    auto* btn = static_cast<CCMenuItemToggler*>(p);
    bool state = !btn->isOn();

    bool isSafeMode = false;

    switch (p->getTag()) {
        case TAG_2P: isSafeMode = true; settingFlags.twoPlayerMode = state; break;
        case TAG_COLLISION: isSafeMode = true; settingFlags.collision = state; break;
        case TAG_OPEN_INV: settingFlags.publicInvites = state; break;
        case TAG_PRIVATE: settingFlags.isHidden = state; break;
        case TAG_DEATHLINK: settingFlags.deathlink = state; break;
    }

    auto& settings = GlobedSettings::get();

    if (isSafeMode && state && !settings.flags.seenRoomOptionsSafeModeNotice) {
        settings.flags.seenRoomOptionsSafeModeNotice = true;
        this->showSafeModePopup(true);
    }

    if (p->getTag() == TAG_DEATHLINK && settingFlags.deathlink && settingFlags.twoPlayerMode) {
        settingFlags.twoPlayerMode = false;
        static_cast<CCMenuItemToggler*>(m_mainLayer->getChildByIDRecursive("2-player-mode"_spr))->toggle(false);
    } else if (p->getTag() == TAG_2P && settingFlags.twoPlayerMode && settingFlags.deathlink) {
        settingFlags.deathlink = false;
        static_cast<CCMenuItemToggler*>(m_mainLayer->getChildByIDRecursive("deathlink"_spr))->toggle(false);
    }

    bool enableSafeModeDot = (settingFlags.twoPlayerMode || settingFlags.collision);

    safeModeBtn->getChildByType<CCSprite>(0)->setColor(enableSafeModeDot ? ccORANGE : ccGREEN);
}

void CreateRoomPopup::showSafeModePopup(bool firstTime) {
    auto getSafeModeString = [&]() -> std::string {
        std::string mods;

        if (this->settingFlags.twoPlayerMode) {
            mods += "2-Player Mode";
        }

        if (this->settingFlags.collision) {
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

    if (!settingFlags.twoPlayerMode && !settingFlags.collision) {
        PopupManager::get().alert("Safe Mode", "<cy>Safe Mode</c> is <cg>not enabled</c> with these room settings. You are able to make progress on levels while in this room.").showInstant();
        return;
    }

    PopupManager::get().alert(
        "Safe Mode",
        firstTime
            ? "This setting enables <cy>safe mode</c>, which means you won't be able to make progress on levels while in this room."
            : getSafeModeString()
    ).showInstant();
}

void CreateRoomPopup::showRoomNameWarnPopup() {
    PopupManager::get().alert(
        "Note",

        "Room names should be clear and appropriate. "
        "Creating a room with <cy>advertisements</c> or <cr>profanity</c> in its name may lead to a <cy>closure of the room</c>, "
        "or in some cases a <cr>ban</c>. The same rules apply for <cj>hidden rooms</c> too."
    ).showInstant();
}

CreateRoomPopup* CreateRoomPopup::create(RoomLayer* parent) {
    auto ret = new CreateRoomPopup;
    if (ret->initAnchored(POPUP_WIDTH, POPUP_HEIGHT, parent)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}