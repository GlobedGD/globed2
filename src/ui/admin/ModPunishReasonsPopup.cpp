#include "ModPunishReasonsPopup.hpp"

#include <core/net/NetworkManagerImpl.hpp>
#include <cue/ListNode.hpp>
#include <UIBuilder.hpp>
#include <AdvancedLabel.hpp>

using namespace geode::prelude;

namespace globed {

static constexpr CCSize LIST_SIZE = { 360.f, 185.f };
static constexpr float CELL_HEIGHT = 28.f;

namespace {

class Cell : public CCMenu {
public:
    static Cell* create(ModPunishReasonsPopup* popup, std::string&& reason) {
        auto ret = new Cell;
        ret->m_popup = popup;
        ret->m_reason = std::move(reason);
        ret->init();
        ret->autorelease();
        return ret;
    }

private:
    ModPunishReasonsPopup* m_popup;
    std::string m_reason;

    bool init() {
        CCMenu::init();

        static constexpr float width = LIST_SIZE.width;
        this->setContentSize({ width, CELL_HEIGHT });

        Build<Label>::create(m_reason, "bigFont.fnt")
            .with([&](auto label) {
                label->limitLabelWidth(width - 32.f, 0.45f, 0.1f);
            })
            .anchorPoint(0.f, 0.5f)
            .pos(5.f, CELL_HEIGHT / 2.f)
            .parent(this);

        Build<CCSprite>::createSpriteName("btn_chatHistory_001.png")
            .with([&](auto spr) {
                cue::rescaleToMatch(spr, CELL_HEIGHT - 4.f);
            })
            .intoMenuItem([this] {
                m_popup->invokeCallback(m_reason);
            })
            .with([&](CCMenuItemSpriteExtra* btn) {
                btn->setPosition({ width - 5.f - btn->getScaledContentWidth() / 2.f, CELL_HEIGHT / 2.f });
            })
            .parent(this);

        return true;
    }
};

}

bool ModPunishReasonsPopup::setup(UserPunishmentType type) {
    this->setTitle("Common Reasons");

    auto list = Build(cue::ListNode::create(LIST_SIZE))
        .pos(this->center())
        .parent(m_mainLayer)
        .collect();

    auto allReasons = NetworkManagerImpl::get().getModPunishReasons();
    std::span<std::string> reasons;

    switch (type) {
        case UserPunishmentType::Ban:
            reasons = allReasons.banReasons;
            break;

        case UserPunishmentType::Mute:
            reasons = allReasons.muteReasons;
            break;

        case UserPunishmentType::RoomBan:
            reasons = allReasons.roomBanReasons;
            break;
    }

    for (auto& reason : reasons) {
        list->addCell(Cell::create(this, std::move(reason)));
    }

    list->scrollToTop();

    return true;
}

}
