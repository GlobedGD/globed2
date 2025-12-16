#include "ModPunishReasonsPopup.hpp"
#include <core/net/NetworkManagerImpl.hpp>
#include <ui/misc/InputPopup.hpp>

#include <cue/ListNode.hpp>
#include <UIBuilder.hpp>
#include <AdvancedLabel.hpp>

using namespace geode::prelude;

namespace globed {

static constexpr CCSize LIST_SIZE = { 360.f, 185.f };
static constexpr float CELL_HEIGHT = 28.f;

namespace {

class Cell : public CCNode {
public:
    static Cell* create(ModPunishReasonsPopup* popup, std::string&& reason, bool custom) {
        auto ret = new Cell;
        ret->m_popup = popup;
        ret->m_custom = custom;
        ret->init(reason);
        ret->autorelease();
        return ret;
    }

    void updateText(const std::string& text) {
        m_reason = text;
        m_label->setString(text);
        m_label->limitLabelWidth(LIST_SIZE.width - 84.f, 0.55f, 0.1f);
    }

private:
    ModPunishReasonsPopup* m_popup;
    CCMenu* m_rightMenu;
    Label* m_label;
    std::string m_reason;
    bool m_custom;

    static constexpr float BUTTON_SIZE = CELL_HEIGHT * 0.8f;

    bool init(const std::string& reason) {
        CCNode::init();

        static constexpr float width = LIST_SIZE.width;
        this->setContentSize({ width, CELL_HEIGHT });

        m_rightMenu = Build<CCMenu>::create()
            .layout(
                SimpleRowLayout::create()
                    ->setGap(3.f)
                    ->setMainAxisAlignment(MainAxisAlignment::Start)
                    ->setMainAxisDirection(AxisDirection::RightToLeft)
            )
            .contentSize({ 72.f, CELL_HEIGHT })
            .ignoreAnchorPointForPos(false)
            .anchorPoint(1.f, 0.5f)
            .pos({ width - 5.f, CELL_HEIGHT / 2.f })
            .parent(this);

        m_label = Build<Label>::create("", "bigFont.fnt")
            .anchorPoint(0.f, 0.5f)
            .pos(5.f, CELL_HEIGHT / 2.f)
            .parent(this);

        this->updateText(reason);

        Build<CCSprite>::createSpriteName("btn_chatHistory_001.png")
            .with([&](auto spr) {
                cue::rescaleToMatch(spr, BUTTON_SIZE);
            })
            .intoMenuItem([this] {
                m_popup->invokeCallback(m_reason);
            })
            .parent(m_rightMenu);

        if (m_custom) {
            this->addCustomButtons();
        }

        m_rightMenu->updateLayout();

        return true;
    }

    void addCustomButtons() {
        // edit button
        auto spr = Build<CCSprite>::create("pencil.png"_spr)
            .collect();

        Build<CircleButtonSprite>::create(spr)
            .with([&](auto* item) { cue::rescaleToMatch(item, BUTTON_SIZE); })
            .intoMenuItem([this](auto) {
                m_popup->openEditReasonPopup(this->getIndex(), m_reason);
            })
            .parent(m_rightMenu);

        // delete button
        Build<CCSprite>::createSpriteName("GJ_deleteSongBtn_001.png")
            .with([&](auto spr) { cue::rescaleToMatch(spr, BUTTON_SIZE); })
            .intoMenuItem([this] {
                m_popup->deleteCustomReason(this->getIndex());
            })
            .parent(m_rightMenu);
    }

    size_t getIndex() {
        // this is hacky :sob:
        return this->getParent()->getZOrder();
    }
};

}

static std::string_view keyForCustomReasons(UserPunishmentType type) {
    switch (type) {
        case UserPunishmentType::Ban:
            return "core.mod.custom-ban-reasons";

        case UserPunishmentType::Mute:
            return "core.mod.custom-mute-reasons";

        case UserPunishmentType::RoomBan:
            return "core.mod.custom-room-ban-reasons";
    }

    return {};
}

bool ModPunishReasonsPopup::setup(UserPunishmentType type) {
    m_type = type;

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

    m_serverList = this->makeList(reasons, false);

    // get custom reasons
    auto customReasons = this->getCustomReasons();
    m_customList = this->makeList(customReasons, true);

    // button for switching to custom reasons
    auto btnContainer = Build<CCMenu>::create()
        .layout(
            SimpleRowLayout::create()->setGap(3.f)
                ->setMainAxisScaling(AxisScaling::Fit)
                ->setCrossAxisScaling(AxisScaling::Fit)
        )
        .pos(this->fromBottom(21.f))
        .parent(m_mainLayer)
        .collect();

    Build(CCMenuItemExt::createTogglerWithStandardSprites(0.6f, [this](auto toggler) {
        bool on = !toggler->isOn();
        this->toggleCustomReasons(on);
    }))
        .parent(btnContainer);

    Build<Label>::create("Custom", "bigFont.fnt")
        .scale(0.45f)
        .parent(btnContainer);

    btnContainer->updateLayout();
    cue::attachBackground(btnContainer);

    this->toggleCustomReasons(false);

    return true;
}

cue::ListNode* ModPunishReasonsPopup::makeList(std::span<std::string> reasons, bool custom) {
    auto list = Build(cue::ListNode::create(LIST_SIZE))
        .pos(this->center())
        .parent(m_mainLayer)
        .collect();

    list->setJustify(cue::Justify::Center);

    for (auto& reason : reasons) {
        list->addCell(Cell::create(this, std::move(reason), custom));
    }

    // add a plus sign to create new custom reasons
    if (custom) {
        auto plusBtn = Build<CCSprite>::createSpriteName("GJ_plusBtn_001.png")
            .with([&](auto btn) { cue::rescaleToMatch(btn, CCSize{CELL_HEIGHT, CELL_HEIGHT}); })
            .intoMenuItem([this] {
                this->createBlankReason();
            })
            .scaleMult(1.1f)
            .collect();

        auto menu = Build<CCMenu>::create()
            .contentSize(plusBtn->getScaledContentSize())
            .ignoreAnchorPointForPos(false)
            .intoNewChild(plusBtn)
            .pos(plusBtn->getScaledContentSize() / 2.f)
            .intoParent()
            .collect();

        list->addCell(menu);
    }

    list->scrollToTop();
    return list;
}

std::vector<std::string> ModPunishReasonsPopup::getCustomReasons() {
    auto key = keyForCustomReasons(m_type);
    return globed::value<std::vector<std::string>>(key).value_or(std::vector<std::string>{});
}

void ModPunishReasonsPopup::saveCustomReasons(const std::vector<std::string>& reasons) {
    auto key = keyForCustomReasons(m_type);
    globed::setValue(key, reasons);
}

void ModPunishReasonsPopup::createBlankReason() {
    auto cell = Cell::create(this, "New reason", true);

    // create it before the plus button
    m_customList->insertCell(cell, m_customList->size() - 1);
    this->commitNewReason("New reason");
}

void ModPunishReasonsPopup::toggleCustomReasons(bool custom) {
    this->setTitle(
        custom ? "Custom Reasons" : "Common Reasons",
        "goldFont.fnt",
        0.7f,
        17.5f
    );

    m_customList->setVisible(custom);
    m_serverList->setVisible(!custom);
}

void ModPunishReasonsPopup::openEditReasonPopup(size_t index, const std::string& currentReason) {
    auto popup = InputPopup::create("chatFont.fnt");
    popup->setDefaultText(currentReason);
    popup->setPlaceholder("Reason");
    popup->setMaxCharCount(128);
    popup->setCallback([this, index](auto outcome) {
        if (outcome.cancelled) return;

        this->commitEditReason(index, outcome.text);
        auto cell = static_cast<Cell*>(m_customList->getCell(index)->getInner());
        cell->updateText(outcome.text);
    });
    popup->show();
}

void ModPunishReasonsPopup::commitNewReason(std::string reason) {
    auto reasons = this->getCustomReasons();
    reasons.push_back(std::move(reason));
    this->saveCustomReasons(reasons);
}

void ModPunishReasonsPopup::commitEditReason(size_t index, std::string reason) {
    auto reasons = this->getCustomReasons();
    if (index >= reasons.size()) {
        log::error("Tried to edit custom reason at invalid index {}", index);
        return;
    }

    reasons[index] = std::move(reason);
    this->saveCustomReasons(reasons);
}


void ModPunishReasonsPopup::commitDeleteReason(size_t index) {
    auto reasons = this->getCustomReasons();
    if (index >= reasons.size()) {
        log::error("Tried to delete custom reason at invalid index {}", index);
        return;
    }

    reasons.erase(reasons.begin() + index);
    this->saveCustomReasons(reasons);
}

void ModPunishReasonsPopup::deleteCustomReason(size_t index) {
    this->commitDeleteReason(index);
    m_customList->removeCell(index);
}

}
