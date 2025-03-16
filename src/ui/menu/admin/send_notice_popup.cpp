#include "send_notice_popup.hpp"

#include <net/manager.hpp>
#include <managers/error_queues.hpp>
#include <util/ui.hpp>
#include <util/misc.hpp>
#include <util/format.hpp>

using namespace geode::prelude;

bool AdminSendNoticePopup::setup(std::string_view message) {
    this->message = message;

    auto sizes = util::ui::getPopupLayoutAnchored(m_size);

    auto* rootLayout = Build<CCNode>::create()
        .pos(sizes.center)
        .anchorPoint(0.5f, 0.5f)
        .layout(ColumnLayout::create()->setGap(5.f)->setAxisReverse(true))
        .parent(m_mainLayer)
        .id("send-notice-root-layout"_spr)
        .collect();

    // specific user
    auto* userLayout = Build<CCMenu>::create()
        .layout(RowLayout::create()->setGap(5.f))
        .pos(0.f, 0.f)
        .parent(rootLayout)
        .collect();

    Build<TextInput>::create(m_size.width * 0.44f, "User ID / Name", "chatFont.fnt")
        .parent(userLayout)
        .store(userInput);

    Build(CCMenuItemExt::createTogglerWithStandardSprites(0.7f, [](auto) {}))
        .parent(userLayout)
        .store(userCanReplyCheckbox);

    Build<CCLabelBMFont>::create("Can reply?", "bigFont.fnt")
        .with([](CCLabelBMFont* a) {
            a->setLayoutOptions(AxisLayoutOptions::create()->setAutoScale(false));
        })
        .scale(0.3f)
        .parent(userLayout);

    userInput->setFilter(std::string(util::misc::STRING_ALPHANUMERIC) + "@ ");
    userInput->setMaxCharCount(16);

    Build<ButtonSprite>::create("Send", "bigFont.fnt", "GJ_button_01.png", 0.5f)
        .intoMenuItem([this](auto) {
            this->commonSend(AdminSendNoticeType::Person);
        })
        .parent(userLayout);

    userLayout->updateLayout();

    // everyone in a room or level
    auto* rlLayout = Build<CCMenu>::create()
        .layout(RowLayout::create()->setGap(5.f))
        .pos(0.f, 0.f)
        .parent(rootLayout)
        .collect();

    Build<TextInput>::create(m_size.width * 0.35, "Room ID (optional)", "chatFont.fnt")
        .parent(rlLayout)
        .store(roomInput);

    roomInput->setCommonFilter(CommonFilter::Uint);
    roomInput->setMaxCharCount(7);

    Build<TextInput>::create(m_size.width * 0.35, "Level ID (optional)", "chatFont.fnt")
        .parent(rlLayout)
        .store(levelInput);

    levelInput->setCommonFilter(CommonFilter::Uint);
    levelInput->setMaxCharCount(12);

    Build<ButtonSprite>::create("Send", "bigFont.fnt", "GJ_button_01.png", 0.5f)
        .intoMenuItem([this](auto) {
            this->commonSend(AdminSendNoticeType::RoomOrLevel);
        })
        .pos(0.f, 0.f)
        .parent(rlLayout);

    rlLayout->updateLayout();

    // everyone on the server
    CCMenuItemSpriteExtra* everyoneBtn;
    Build<ButtonSprite>::create("Send to everyone", "bigFont.fnt", "GJ_button_01.png", 0.7f)
        .intoMenuItem([this](auto) {
            this->commonSend(AdminSendNoticeType::Everyone);
        })
        .store(everyoneBtn)
        .intoNewParent(CCMenu::create())
        .layout(RowLayout::create())
        .parent(rootLayout);

    rootLayout->setContentSize(CCPoint{
        m_size.width * 0.8f,
        10.f + userLayout->getScaledContentSize().height + rlLayout->getScaledContentSize().height + everyoneBtn->getScaledContentSize().height
    });

    rootLayout->updateLayout();

    return true;
}

void AdminSendNoticePopup::commonSend(AdminSendNoticeType type) {
    uint32_t roomId = 0;
    int levelId = 0;
    bool canReply = false;

    if (type == AdminSendNoticeType::RoomOrLevel) {
        levelId = util::format::parse<int>(levelInput->getString()).value_or(0);
        roomId = util::format::parse<uint32_t>(roomInput->getString()).value_or(0);
    } else {
        canReply = userCanReplyCheckbox->isOn();
    }

    auto packet = AdminSendNoticePacket::create(type, roomId, levelId, userInput->getString(), message, canReply);
    NetworkManager::get().send(packet);
}

AdminSendNoticePopup* AdminSendNoticePopup::create(std::string_view message) {
    auto* ret = new AdminSendNoticePopup;
    if (ret->initAnchored(POPUP_WIDTH, POPUP_HEIGHT, message)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}