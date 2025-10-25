#include "SaveSlotSwitcherPopup.hpp"

#include <globed/core/PopupManager.hpp>
#include <globed/core/SettingsManager.hpp>
#include <ui/misc/InputPopup.hpp>

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

const CCSize SaveSlotSwitcherPopup::POPUP_SIZE = {360.f, 280.f};
static const CCSize LIST_SIZE = { 306.f, 210.f };
static constexpr float CELL_HEIGHT = 36.f;

namespace {
class Cell : public CCNode {
public:
    static Cell* create(SaveSlotMeta meta, SaveSlotSwitcherPopup* popup) {
        auto ret = new Cell;
        ret->m_meta = std::move(meta);
        ret->m_popup = popup;

        if (ret->init()) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }

private:
    SaveSlotMeta m_meta;
    SaveSlotSwitcherPopup* m_popup;

    bool init() {
        CCNode::init();

        this->setContentSize({LIST_SIZE.width, CELL_HEIGHT});

        Build<CCLabelBMFont>::create(m_meta.name.c_str(), "bigFont.fnt")
            .anchorPoint(0.f, 0.5f)
            .limitLabelWidth(220.f, 0.6f, 0.01f)
            .pos(8.f, CELL_HEIGHT / 2.f)
            .parent(this);

        auto btnMenu = Build<CCMenu>::create()
            .layout(RowLayout::create()
                        ->setAxisReverse(true)
                        ->setAxisAlignment(AxisAlignment::End)
                        ->setGap(3.f))
            .contentSize(this->getContentSize() + CCSize{-16.f, 0.f})
            .pos(this->getContentSize() / 2.f)
            .parent(this)
            .collect();

        // button for switching to the slot
        auto texture = m_meta.active ? "GJ_selectSongOnBtn_001.png" : "GJ_playBtn2_001.png";

        auto switchBtn = Build<CCSprite>::createSpriteName(texture)
            .with([&](auto btn) { cue::rescaleToMatch(btn, {CELL_HEIGHT * 0.75f, CELL_HEIGHT * 0.75f}); })
            .intoMenuItem([this](auto) {
                if (m_meta.active) return;

                SettingsManager::get().switchToSaveSlot(m_meta.id);
                m_popup->refreshList();
                m_popup->invokeSwitchCallback();
            })
            .parent(btnMenu)
            .collect();

        // button for renaming the slot
        Build(CircleButtonSprite::createWithSprite("pencil.png"_spr))
            .with([&](auto btn) { cue::rescaleToMatch(btn, switchBtn); })
            .intoMenuItem([this](auto) {
                auto popup = InputPopup::create("chatFont.fnt");
                popup->setTitle("Rename Profile");
                popup->setPlaceholder("Profile name");
                popup->setMaxCharCount(24);
                popup->setDefaultText(m_meta.name);
                popup->setCommonFilter(CommonFilter::Any);
                popup->setCallback([this](InputPopupOutcome outcome) {
                    if (outcome.cancelled) return;

                    log::debug("bruh renaming save slot {} to {}", m_meta.id, outcome.text);
                    SettingsManager::get().renameSaveSlot(m_meta.id, outcome.text);
                    m_popup->refreshList();
                });
                popup->show();
            })
            .parent(btnMenu);

        // button for deleting the slot
        if (!m_meta.active) {
            Build<CCSprite>::createSpriteName("GJ_deleteBtn_001.png")
                .with([&](auto spr) { cue::rescaleToMatch(spr, switchBtn); })
                .intoMenuItem([this](auto) {
                    SettingsManager::get().deleteSaveSlot(m_meta.id);
                    m_popup->refreshList();
                })
                .parent(btnMenu);
        }

        btnMenu->updateLayout();

        return true;
    }
};
}

bool SaveSlotSwitcherPopup::setup() {
    this->setTitle("Setting Profiles");

    m_list = Build(cue::ListNode::create(LIST_SIZE))
        .pos(this->fromCenter(0.f, -15.f))
        .parent(m_mainLayer);
    m_list->setJustify(cue::Justify::Center);

    this->refreshList(true);

    return true;
}

void SaveSlotSwitcherPopup::refreshList(bool scrollToTop) {
    auto slots = SettingsManager::get().getSaveSlots();
    auto scpos = m_list->getScrollPos();

    m_list->clear();

    for (auto& slot : slots) {
        m_list->addCell(Cell::create(std::move(slot), this));
    }

    auto plusBtn = Build<CCSprite>::createSpriteName("GJ_plusBtn_001.png")
        .with([&](auto btn) { cue::rescaleToMatch(btn, CCSize{CELL_HEIGHT, CELL_HEIGHT}); })
        .intoMenuItem([this] {
            SettingsManager::get().createSaveSlot();
            this->refreshList();
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

    m_list->addCell(menu);

    if (scrollToTop) {
        m_list->scrollToTop();
    } else {
        m_list->setScrollPos(scpos);
    }
}

void SaveSlotSwitcherPopup::invokeSwitchCallback() {
    if (m_callback) {
        m_callback();
    }
}

}