#include "direct_connect_popup.hpp"

#include <regex>

#include "server_switcher_popup.hpp"
#include <net/network_manager.hpp>
#include <managers/central_server.hpp>
#include <managers/game_server.hpp>
#include <managers/error_queues.hpp>
#include <util/misc.hpp>

using namespace geode::prelude;

bool DirectConnectionPopup::setup(ServerSwitcherPopup* parent) {
    this->parent = parent;
    this->setTitle("Direct connection");

    float popupCenter = CCDirector::get()->getWinSize().width / 2;

    // address hint
    Build<CCLabelBMFont>::create("Server address", "bigFont.fnt")
        .scale(0.3f)
        .pos(popupCenter, POPUP_HEIGHT - 20.f)
        .parent(m_mainLayer)
        .id("direct-connection-addr-hint"_spr);

    // address input node
    Build<InputNode>::create(POPUP_WIDTH * 0.75f, "127.0.0.1:41001", "chatFont.fnt", std::string(util::misc::STRING_URL), 21)
        .pos(popupCenter, POPUP_HEIGHT - 40.f)
        .parent(m_mainLayer)
        .id("direct-connection-addr"_spr)
        .store(addressNode);

    Build<ButtonSprite>::create("Connect", "bigFont.fnt", "GJ_button_01.png", 0.8f)
        .intoMenuItem([this](auto) {
            // this is scary
            static std::regex pattern(R"(^(?!(?:https?):\/\/)(?:(?:[0-9]{1,3}\.){3}[0-9]{1,3}|(?:[a-zA-Z0-9-]+\.)+[a-zA-Z]{2,})(?::\d+)?$)");

            std::string addr = this->addressNode->getString();

            if (addr.empty() || !std::regex_match(addr, pattern)) {
                FLAlertLayer::create("Error", "You passed an invalid address. It must be an IPv4 address or a domain name with an optional port at the end. IPv4 Example: <cy>144.72.254.213:41001</c> - Domain Example: <cy>globed.example.com:41001</c>)", "Ok")->show();
                return;
            }

            auto& gsm = GameServerManager::get();
            auto result = gsm.addServer(GameServerManager::STANDALONE_ID, "Server", addr, "unknown");
            if (result.isErr()) {
                FLAlertLayer::create("Error", "Failed to connect to the server: " + result.unwrapErr(), "Ok")->show();
                return;
            }

            auto& csm = CentralServerManager::get();
            csm.setStandalone();
            csm.recentlySwitched = true;

            gsm.clearAllExcept(GameServerManager::STANDALONE_ID);
            gsm.pendingChanges = true;
            gsm.saveStandalone(addr);
            gsm.clearCache();

            auto& nm = NetworkManager::get();
            result = nm.connectStandalone();
            if (result.isErr()) {
                FLAlertLayer::create("Error", result.unwrapErr(), "Ok")->show();
            }

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
