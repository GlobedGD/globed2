#include "save_slot_switcher_popup.hpp"

#include <managers/settings.hpp>
#include <managers/popup.hpp>
#include <ui/general/ask_input_popup.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;
using SaveSlot = GlobedSettings::SaveSlot;

bool SaveSlotSwitcherPopup::setup() {
    auto rlayout = util::ui::getPopupLayoutAnchored(m_size);
    this->setTitle("Setting Profiles");

    Build(GlobedListLayer<ListCell>::createForComments(LIST_WIDTH, LIST_HEIGHT))
        .pos(rlayout.fromCenter(0.f, -15.f))
        .parent(m_mainLayer)
        .store(list);

    this->refreshList(true);

    return true;
}

void SaveSlotSwitcherPopup::refreshList(bool scrollToTop) {
    this->saveSlots = GlobedSettings::get().getSaveSlots();
    auto lastScrollPos = this->list->getScrollPos();

    list->removeAllCells(false);

    for (auto& slot : this->saveSlots) {
        list->addCellFast(&slot, this);
    }

    // button for adding a new slot
    list->addCell(nullptr, this);

    list->sort([](ListCell* cell1, ListCell* cell2) {
        // the add slot button always goes last
        if (!cell1->saveSlot) {
            return false;
        }

        // sort by index
        return cell1->saveSlot->index < cell2->saveSlot->index;
    });

    list->forceUpdate();

    if (scrollToTop) {
        list->scrollToTop();
    } else {
        list->scrollToPos(lastScrollPos);
    }
}

SaveSlotSwitcherPopup* SaveSlotSwitcherPopup::create() {
    auto ret = new SaveSlotSwitcherPopup;
    if (ret->initAnchored(POPUP_WIDTH, POPUP_HEIGHT)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

bool SaveSlotSwitcherPopup::ListCell::init(SaveSlot* slot) {
    this->setContentSize(CCSize{LIST_WIDTH, CELL_HEIGHT});
    auto rlayout = util::ui::getNodeLayout(this->getContentSize());

    Build<CCLabelBMFont>::create(slot->name.c_str(), "bigFont.fnt")
        .anchorPoint(0.f, 0.5f)
        .limitLabelWidth(220.f, 0.6f, 0.01f)
        .pos(rlayout.fromLeft(8.f))
        .parent(this);

    auto* buttonMenu = Build<CCMenu>::create()
        .layout(
            RowLayout::create()
                    ->setAxisReverse(true)
                    ->setAxisAlignment(AxisAlignment::End)
                    ->setGap(3.f)
        )
        .contentSize(this->getContentSize() + CCSize{-16.f, 0.f})
        .pos(this->getContentSize() / 2.f)
        .parent(this)
        .collect();

    bool active = slot->index == GlobedSettings::get().getSelectedSlot();

    // button for switching to the slot
    auto texture = active ? "GJ_selectSongOnBtn_001.png" : "GJ_playBtn2_001.png";

    auto switchBtn = Build<CCSprite>::createSpriteName(texture)
        .with([&](auto* item) {
            util::ui::rescaleToMatch(item, {CELL_HEIGHT * 0.75f, CELL_HEIGHT * 0.75f});
        })
        .intoMenuItem([this, slot, active](auto) {
            if (active) {
                return;
            }

            auto& settings = GlobedSettings::get();
            settings.switchSlot(slot->index);

            this->popup->refreshList();
        })
        .parent(buttonMenu)
        .collect();

    // button for renaming the slot
    auto spr = Build<CCSprite>::createSpriteName("pencil.png"_spr)
        .collect();

    Build<CircleButtonSprite>::create(spr)
        .with([&](auto* item) {
            util::ui::rescaleToMatch(item, switchBtn);
        })
        .intoMenuItem([this, slot, active](auto) {
            AskInputPopup::create("Rename Profile", [this, slot, active](std::string_view name) {
                GlobedSettings::get().renameSlot(slot->index, name);
                this->popup->refreshList();
            }, 64, slot->name, util::misc::STRING_PRINTABLE_INPUT)->show();
        })
        .parent(buttonMenu);

    // button for deleting the slot
    if (!active) {
        Build<CCSprite>::createSpriteName("GJ_deleteBtn_001.png")
            .with([&](auto* item) {
                util::ui::rescaleToMatch(item, switchBtn);
            })
            .intoMenuItem([this, slot, active](auto) {
                auto& settings = GlobedSettings::get();
                settings.deleteSlot(slot->index);

                this->popup->refreshList();
            })
            .parent(buttonMenu);
    }

    buttonMenu->updateLayout();

    return true;
}

bool SaveSlotSwitcherPopup::ListCell::initAddButton() {
    this->setContentSize(CCSize{LIST_WIDTH, CELL_HEIGHT});
    auto rlayout = util::ui::getNodeLayout(this->getContentSize());

    auto* buttonMenu = Build<CCMenu>::create()
        .layout(RowLayout::create())
        .contentSize(this->getContentSize() + CCSize{-16.f, 0.f})
        .pos(this->getContentSize() / 2.f)
        .parent(this)
        .collect();

    Build<CCSprite>::createSpriteName("GJ_plusBtn_001.png")
        .with([&](auto* node) {
            util::ui::rescaleToMatch(node, {CELL_HEIGHT * 0.8f, CELL_HEIGHT * 0.8f});
        })
        .intoMenuItem([this](auto) {
            auto& settings = GlobedSettings::get();

            auto res = settings.createSlot();
            if (!res) {
                PopupManager::get().alertFormat("Error", "Failed to create new save slot: <cy>{}</c>", res.unwrapErr()).showInstant();
                return;
            }

            this->popup->refreshList();
        })
        .scaleMult(1.15f)
        .parent(buttonMenu);

    buttonMenu->updateLayout();

    return true;
}

SaveSlotSwitcherPopup::ListCell* SaveSlotSwitcherPopup::ListCell::create(SaveSlot* slot, SaveSlotSwitcherPopup* popup) {
    auto ret = new ListCell;
    ret->popup = popup;

    bool initRes;

    if (slot == nullptr) {
        initRes = ret->initAddButton();
    } else {
        initRes = ret->init(slot);
    }

    if (initRes) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}
