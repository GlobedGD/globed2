#include "relay_cell.hpp"

#include "relay_switch_popup.hpp"
#include <managers/error_queues.hpp>
#include <managers/game_server.hpp>
#include <net/manager.hpp>

using namespace geode::prelude;

bool RelayCell::init(const ServerRelay& relay, RelaySwitchPopup* parent) {
    if (!CCLayer::init()) return false;

    m_data = relay;
    m_parent = parent;

    auto& gsm = GameServerManager::get();

    bool active = gsm.getActiveRelayId() == m_data.id;

    // select button
    Build<CCSprite>::createSpriteName("GJ_selectSongBtn_001.png")
        .scale(0.8f)
        .intoMenuItem([this](auto) {
            log::info("Enabled relay: {}", m_data.address);
            GameServerManager::get().setActiveRelay(m_data.id);

            m_parent->refreshList();
        })
        .visible(!active)
        .id("select-device-btn"_spr)
        .pos(RelaySwitchPopup::LIST_WIDTH - 5.f, CELL_HEIGHT / 2)
        .store(m_btnSelect)
        .intoNewParent(CCMenu::create())
        .pos(0.f, 0.f)
        .parent(this);

    // selected sprite
    Build<CCSprite>::createSpriteName("GJ_selectSongOnBtn_001.png")
        .scale(0.8f)
        .intoMenuItem([this](auto) {
            GameServerManager::get().setActiveRelay("");

            m_parent->refreshList();
        })
        .visible(active)
        .id("selected-device-btn"_spr)
        .pos(RelaySwitchPopup::LIST_WIDTH - 5.f, CELL_HEIGHT / 2)
        .store(m_btnSelected)
        .intoNewParent(CCMenu::create())
        .pos(0.f, 0.f)
        .parent(this);

    m_btnSelect->setPositionX(m_btnSelect->getPositionX() - m_btnSelect->getScaledContentSize().width / 2);
    m_btnSelected->setPositionX(m_btnSelected->getPositionX() - m_btnSelected->getScaledContentSize().width / 2);

    // name
    Build<CCLabelBMFont>::create(m_data.name.c_str(), "goldFont.fnt")
        .limitLabelWidth(290.f, 0.8f, 0.1f)
        .anchorPoint(0.f, 0.5f)
        .pos(5.f, CELL_HEIGHT / 2)
        .parent(this)
        .id("device-name-label"_spr);

    return true;
}

void RelayCell::refresh() {
    auto& gsm = GameServerManager::get();

    bool active = gsm.getActiveRelayId() == m_data.id;

    m_btnSelect->setVisible(!active);
    m_btnSelected->setVisible(active);
}

RelayCell* RelayCell::create(const ServerRelay& relay, RelaySwitchPopup* parent) {
    auto ret = new RelayCell;
    if (ret->init(relay, parent)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}
