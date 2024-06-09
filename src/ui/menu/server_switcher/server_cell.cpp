#include "server_cell.hpp"

#include "server_switcher_popup.hpp"
#include "add_server_popup.hpp"
#include <net/manager.hpp>
#include <managers/game_server.hpp>
#include <managers/account.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

bool CentralServerListCell::init(const CentralServer& data, int index, ServerSwitcherPopup* parent) {
    if (!CCLayer::init()) return false;

    this->data = data;
    this->parent = parent;
    this->index = index;

    Build<CCLabelBMFont>::create(data.name.c_str(), "bigFont.fnt")
        .limitLabelWidth(170.f, 0.8f, 0.1f)
        .anchorPoint(0.0f, 0.5f)
        .pos(5.f, CELL_HEIGHT / 2)
        .parent(this)
        .id("server-list-cell-name"_spr)
        .store(nameLabel);

    // button menu

    Build<CCMenu>::create()
        .layout(
            RowLayout::create()
                ->setAutoScale(false)
                ->setGap(5.0f)
                ->setAxisAlignment(AxisAlignment::End)
        )
        .anchorPoint(1.0f, 0.5f)
        .pos(ServerSwitcherPopup::LIST_WIDTH - 10.f, CELL_HEIGHT / 2)
        .parent(this)
        .id("server-list-cell-menu"_spr)
        .store(menu);

    // delete server btn

    Build<CCSprite>::createSpriteName("GJ_deleteBtn_001.png")
        .scale(0.7f)
        .intoMenuItem([this](auto) {
            auto& csm = CentralServerManager::get();
            // don't delete if it's the only server
            if (csm.count() == 1) {
                FLAlertLayer::create("Notice", "You can't delete every single server!", "Ok")->show();
                return;
            }

            csm.removeServer(this->index);

            this->parent->reloadList();
        })
        .parent(menu)
        .id("server-list-cell-delete"_spr)
        .store(btnDelete);

    // modify server btn

    Build<CCSprite>::createSpriteName("GJ_editBtn_001.png")
        .scale(0.4f)
        .intoMenuItem([this](auto) {
            AddServerPopup::create(this->index, this->parent)->show();
        })
        .parent(menu)
        .id("server-list-cell-modify"_spr)
        .store(btnModify);

    // switch to server btn

    auto& csm = CentralServerManager::get();
    bool isActive = csm.getActiveIndex() == index;

    auto btnspr = Build<CCSprite>::createSpriteName(isActive ? "GJ_selectSongOnBtn_001.png" : "GJ_playBtn2_001.png")
        .collect();

    util::ui::rescaleToMatch(btnspr, btnModify);

    Build<CCSprite>(btnspr)
        .intoMenuItem([this, &csm, isActive](auto) {
            if (!isActive) {
                csm.switchRoutine(this->index);
                this->parent->close();
            }
        })
        .parent(menu)
        .id("server-list-cell-switch"_spr)
        .store(btnSwitch);

    menu->updateLayout();

    return true;
}

CentralServerListCell* CentralServerListCell::create(const CentralServer& data, int index, ServerSwitcherPopup* parent) {
    auto ret = new CentralServerListCell;
    if (ret->init(data, index, parent)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}