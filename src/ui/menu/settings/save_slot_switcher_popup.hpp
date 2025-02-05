#pragma once
#include <defs/geode.hpp>

#include <managers/settings.hpp>
#include <ui/general/list/list.hpp>

class SaveSlotSwitcherPopup : public geode::Popup<> {
public:
    static constexpr float POPUP_WIDTH = 360.f;
    static constexpr float POPUP_HEIGHT = 220.f;

    static constexpr float LIST_WIDTH = POPUP_WIDTH * 0.85f;
    static constexpr float LIST_HEIGHT = POPUP_HEIGHT * 0.65f;

    static SaveSlotSwitcherPopup* create();

private:
    using SaveSlot = GlobedSettings::SaveSlot;

    bool setup() override;
    void refreshList(bool scrollToTop = false);

    class ListCell : public cocos2d::CCNode {
        static constexpr float CELL_HEIGHT = 36.f;
        using SaveSlot = GlobedSettings::SaveSlot;

    public:
        SaveSlot* saveSlot = nullptr;
        SaveSlotSwitcherPopup* popup = nullptr;

        static ListCell* create(SaveSlot*, SaveSlotSwitcherPopup* popup);

    private:
        bool init(SaveSlot*);
        bool initAddButton();
    };

    GlobedListLayer<ListCell>* list;
    std::vector<SaveSlot> saveSlots;
};
