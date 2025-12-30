#include "ModNoticeSetupPopup.hpp"
#include <globed/core/PopupManager.hpp>
#include <core/net/NetworkManagerImpl.hpp>

#include <UIBuilder.hpp>
#include <cue/Util.hpp>

using namespace geode::prelude;

namespace {
namespace btn {
    constexpr int User = 1;
    constexpr int Room = 2;
    constexpr int Level = 3;
    constexpr int Global = 4;
}
}

namespace globed {

const CCSize ModNoticeSetupPopup::POPUP_SIZE = {320.f, 200.f};

bool ModNoticeSetupPopup::setup() {
    this->setTitle("Send Notice");

    m_messageInput = Build<TextInput>::create(270.f, "Message", "chatFont.fnt")
        .pos(this->fromCenter(0.f, 44.f))
        .parent(m_mainLayer);
    m_messageInput->setCommonFilter(CommonFilter::Any);

    auto checkboxMenu = Build<CCMenu>::create()
        .layout(RowLayout::create()->setAutoScale(false))
        .anchorPoint(0.5f, 0.5f)
        .ignoreAnchorPointForPos(false)
        .contentSize(262.f, 52.f)
        .id("checkbox-menu")
        .pos(this->fromCenter(0.f, 12.f))
        .parent(m_mainLayer)
        .collect();

    auto addCheckbox = [&](CCMenuItemToggler*& store, int mode) {
        Build(CCMenuItemExt::createTogglerWithStandardSprites(0.5f, [this, mode](auto toggler) {
            bool on = !toggler->isOn();
            this->onSelectMode(mode, on);
        }))
            .store(store)
            .parent(checkboxMenu);

        const char* text;
        switch (mode) {
            case btn::User: text = "User"; break;
            case btn::Room: text = "Room"; break;
            case btn::Level: text = "Level"; break;
            case btn::Global: text = "Everyone"; break;
            default: text = "Unknown";
        }

        Build<CCLabelBMFont>::create(text, "bigFont.fnt")
            .limitLabelWidth(50.f, 0.4f, 0.2f)
            .parent(checkboxMenu);
    };

    addCheckbox(m_userCheckbox, btn::User);
    addCheckbox(m_roomCheckbox, btn::Room);
    addCheckbox(m_levelCheckbox, btn::Level);
    addCheckbox(m_globalCheckbox, btn::Global);
    checkboxMenu->updateLayout();

    cue::attachBackground(checkboxMenu, cue::BackgroundOptions {
        .verticalPadding = 5.f
    });

    auto inputsLayout = RowLayout::create()->setAutoScale(false)->setGap(8.f);
    inputsLayout->ignoreInvisibleChildren(true);

    m_inputsContainer = Build<CCMenu>::create()
        .layout(inputsLayout)
        .anchorPoint(0.5f, 0.5f)
        .ignoreAnchorPointForPos(false)
        .contentSize(270.f, 50.f)
        .pos(this->fromCenter(0.f, -20.f))
        .parent(m_mainLayer)
        .collect();

    Build<TextInput>::create(140.f, "Username / ID", "chatFont.fnt")
        .parent(m_inputsContainer)
        .store(m_userInput);

    Build<TextInput>::create(80.f, "Room ID", "chatFont.fnt")
        .parent(m_inputsContainer)
        .store(m_roomInput);

    Build<TextInput>::create(80.f, "Level ID", "chatFont.fnt")
        .parent(m_inputsContainer)
        .store(m_levelInput);

    Build(CCMenuItemExt::createTogglerWithStandardSprites(0.5f, +[](CCMenuItemToggler* toggler) {}))
        .parent(m_inputsContainer)
        .store(m_canReplyCheckbox);

    Build<CCLabelBMFont>::create("Can reply", "bigFont.fnt")
        .scale(0.4f)
        .parent(m_inputsContainer)
        .store(m_canReplyLabel);

    Build<ButtonSprite>::create("Send", "bigFont.fnt", "GJ_button_01.png", 0.7f)
        .scale(0.85f)
        .intoMenuItem([this] {
            this->submit();
        })
        .scaleMult(1.1f)
        .pos(this->fromBottom(27.f))
        .parent(m_buttonMenu);

    // show sender

    Build(CCMenuItemExt::createTogglerWithStandardSprites(0.7f, +[](CCMenuItemToggler* toggler) {}))
        .parent(m_buttonMenu)
        .pos(this->fromBottom({60.f, 27.f}))
        .store(m_showSenderCheckbox);

    Build<CCLabelBMFont>::create("Show\nsender", "bigFont.fnt")
        .scale(0.35f)
        .parent(m_buttonMenu)
        .pos(this->fromBottom({96.f, 27.f}));

    this->setupUser(0);
    m_userCheckbox->toggle(true);

    return true;
}

void ModNoticeSetupPopup::setupUser(int accountId) {
    this->onSelectMode(btn::User, true);

    if (accountId != 0) {
        m_userInput->setString(fmt::to_string(accountId));
    }
}

void ModNoticeSetupPopup::submit() {
    bool everyone = m_globalCheckbox->isOn();
    bool user = m_userCheckbox->isOn();
    bool room = m_roomCheckbox->isOn();
    bool level = m_levelCheckbox->isOn();

    int roomId = 0, levelId = 0;
    std::string userQuery;

    if (user) {
        userQuery = m_userInput->getString();
    }

    if (room) {
        auto res = geode::utils::numFromString<int>(m_roomInput->getString());
        if (!res || *res < 100000 || *res > 999999) {
            globed::alert("Error", "Invalid room ID");
            return;
        }

        roomId = *res;
    }

    if (level) {
        auto res = geode::utils::numFromString<int>(m_levelInput->getString());
        if (!res || *res < 0) {
            globed::alert("Error", "Invalid level ID");
            return;
        }

        levelId = *res;
    }

    if (!user && !room && !level && !everyone) {
        globed::alert("Error", "Please select at least one mode");
        return;
    }

    auto text = m_messageInput->getString();

    if (everyone) {
        globed::confirmPopup(
            "Confirm",
            "This action will send a <cy>notice</c> to ALL USERS on the server. Are you sure you want to proceed?",
            "Cancel",
            "Send",
            [text = std::move(text)](FLAlertLayer*) {
                NetworkManagerImpl::get().sendAdminNoticeEveryone(text);
            }
        );

        return;
    }

    NetworkManagerImpl::get().sendAdminNotice(text, userQuery, roomId, levelId, m_canReplyCheckbox->isOn(), m_showSenderCheckbox->isOn());

    globed::toastSuccess("Notice sent successfully!");
}

void ModNoticeSetupPopup::onSelectMode(int mode, bool on) {
    m_userInput->setVisible(false);
    m_canReplyCheckbox->setVisible(false);
    m_canReplyLabel->setVisible(false);

    if (!on) {
        if (mode == btn::Room) {
            m_roomInput->setVisible(false);
        } else if (mode == btn::Level) {
            m_levelInput->setVisible(false);
        }

        m_inputsContainer->updateLayout();
        return;
    }

    if (mode == btn::User) {
        m_roomCheckbox->toggle(false);
        m_levelCheckbox->toggle(false);
        m_globalCheckbox->toggle(false);
        m_roomInput->setVisible(false);
        m_levelInput->setVisible(false);

        m_userInput->setVisible(true);
        m_canReplyCheckbox->setVisible(true);
        m_canReplyLabel->setVisible(true);
    } else if (mode == btn::Global) {
        m_userCheckbox->toggle(false);
        m_roomCheckbox->toggle(false);
        m_levelCheckbox->toggle(false);
        m_roomInput->setVisible(false);
        m_levelInput->setVisible(false);
    } else if (mode == btn::Room || mode == btn::Level) {
        m_userCheckbox->toggle(false);
        m_globalCheckbox->toggle(false);
        (mode == btn::Room ? m_roomInput : m_levelInput)->setVisible(true);
    }

    m_inputsContainer->updateLayout();
}

}
