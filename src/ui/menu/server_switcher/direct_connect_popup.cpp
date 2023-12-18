#include "direct_connect_popup.hpp"

#include <UIBuilder.hpp>
#include <regex>

#include "server_switcher_popup.hpp"
#include <net/network_manager.hpp>
#include <managers/central_server.hpp>
#include <managers/game_server.hpp>
#include <managers/error_queues.hpp>

using namespace geode::prelude;

bool DirectConnectionPopup::setup(ServerSwitcherPopup* parent) {
    this->parent = parent;
    this->setTitle("Direct connection");

    float popupCenter = CCDirector::get()->getWinSize().width / 2;

    // address hint
    Build<CCLabelBMFont>::create("Server address", "bigFont.fnt")
        .scale(0.3f)
        .pos(popupCenter, POPUP_HEIGHT - 20.f)
        .id("direct-connection-addr-hint"_spr)
        .parent(m_mainLayer);

    // address input node
    Build<InputNode>::create(POPUP_WIDTH * 0.75f, "127.0.0.1:41001", "chatFont.fnt", std::string("1234567890.:"), 21)
        .pos(popupCenter, POPUP_HEIGHT - 40.f)
        .parent(m_mainLayer)
        .id("direct-connection-addr"_spr)
        .store(addressNode);

    Build<ButtonSprite>::create("Connect", "bigFont.fnt", "GJ_button_01.png", 0.8f)
        .intoMenuItem([this](auto) {
            // this is scary
            static std::regex pattern(R"((\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})(?::(\d{1,5}))?)");

            std::string addr = this->addressNode->getString();

            if (addr.empty() || !std::regex_match(addr, pattern)) {
                FLAlertLayer::create("Error", "Invalid address was passed. It must be an IPv4 address with an optional port at the end (like <cy>127.0.0.1:41001</c>)", "Ok")->show();
                return;
            }

            auto& csm = CentralServerManager::get();
            csm.setStandalone();
            csm.recentlySwitched = true;

            auto& gsm = GameServerManager::get();
            gsm.clear();
            gsm.addServer(GameServerManager::STANDALONE_ID, "Server", addr, "unknown");
            gsm.saveStandalone(addr);
            gsm.pendingChanges = true;

            auto& nm = NetworkManager::get();
            nm.connectStandalone();

            this->parent->reloadList();
            this->onClose(this);
            this->parent->close();
        })
        .id("connect-btn"_spr)
        .intoNewParent(CCMenu::create())
        .id("connect-btn-menu"_spr)
        .pos(popupCenter, 55.f)
        .parent(m_mainLayer);

    return true;
}

DirectConnectionPopup* DirectConnectionPopup::create(ServerSwitcherPopup* parent) {
    auto ret = new DirectConnectionPopup;
    if (ret->init(POPUP_WIDTH, POPUP_HEIGHT, parent)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}