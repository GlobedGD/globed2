#include "relay_switch_popup.hpp"

#include <managers/game_server.hpp>

bool RelaySwitchPopup::setup() {
    this->setTitle("Available Relays");

    auto rlayout = util::ui::getPopupLayoutAnchored(m_size);

    Build(RelayList::createForComments(LIST_WIDTH, LIST_HEIGHT, RelayCell::CELL_HEIGHT))
        .pos(rlayout.center)
        .parent(m_mainLayer)
        .store(m_listLayer);

    auto& gsm = GameServerManager::get();

    for (auto& relay : gsm.getAllRelays()) {
        m_listLayer->addCell(relay, this);
    }

    return true;
}

void RelaySwitchPopup::refreshList() {
    for (auto cell : *m_listLayer) {
        cell->refresh();
    }
}

RelaySwitchPopup* RelaySwitchPopup::create() {
    auto ret = new RelaySwitchPopup;
    if (ret->initAnchored(POPUP_WIDTH, POPUP_HEIGHT)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}
