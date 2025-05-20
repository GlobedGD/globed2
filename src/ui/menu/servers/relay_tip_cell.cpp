#include "relay_tip_cell.hpp"

#include "relay_switch_popup.hpp"
#include <managers/game_server.hpp>
#include <managers/settings.hpp>
#include <managers/popup.hpp>

using namespace geode::prelude;

bool RelayTipCell::init() {
    if (!CCLayer::init()) return false;

    this->setContentSize({CELL_WIDTH, CELL_HEIGHT});

    auto& gsm = GameServerManager::get();

    Build<CCLabelBMFont>::create(fmt::format("{} {} available", gsm.relayCount(), gsm.relayCount() == 1 ? "relay" : "relays").c_str(), "bigFont.fnt")
        .scale(0.6f)
        .anchorPoint(0.f, 0.5f)
        .pos(10.f, CELL_HEIGHT / 2.f + 0.f)
        .parent(this);

    Build<ButtonSprite>::create("View", "bigFont.fnt", "GJ_button_01.png", 0.8f)
        .scale(0.7f)
        .intoMenuItem([] {
            auto& gs = GlobedSettings::get();

            if (!gs.flags.seenRelayNotice) {
                gs.flags.seenRelayNotice = true;

                PopupManager::get().quickPopup(
                    "Note",
                    "Relays can help if you have <cy>connection issues</c>. This functionality is <cp>experimental</c> and might have issues, it is recommended to use it only if you can't connect to the server at all.",
                    "Ok",
                    nullptr,
                    [](auto, bool) {
                        RelaySwitchPopup::create()->show();
                    }
                ).showInstant();
            } else {
                RelaySwitchPopup::create()->show();
            }
        })
        .with([&](auto sprite) {
            sprite->setPosition(CELL_WIDTH - sprite->getScaledContentSize().width / 2.f - 6.f, CELL_HEIGHT / 2.f);
        })
        .intoNewParent(CCMenu::create())
        .pos(0.f, 0.f)
        .contentSize(CELL_WIDTH, CELL_HEIGHT)
        .parent(this);

    return true;
}

RelayTipCell* RelayTipCell::create() {
    auto ret = new RelayTipCell;
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}
