#pragma once

#include <defs/geode.hpp>
#include <ui/general/list/list.hpp>

#include "relay_cell.hpp"

class RelaySwitchPopup : public geode::Popup<> {
public:
    static constexpr float POPUP_WIDTH = 400.f;
    static constexpr float POPUP_HEIGHT = 280.f;
    static constexpr float LIST_WIDTH = 340.f;
    static constexpr float LIST_HEIGHT = 200.f;

    static RelaySwitchPopup* create();
    void refreshList();

private:
    using RelayList = GlobedListLayer<RelayCell>;
    RelayList* m_listLayer;

    bool setup() override;
};
