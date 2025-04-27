#include "direct_connect_popup.hpp"

#include <regex>

#include "server_switcher_popup.hpp"
#include <net/manager.hpp>
#include <managers/central_server.hpp>
#include <managers/game_server.hpp>
#include <managers/error_queues.hpp>
#include <managers/popup.hpp>
#include <net/address.hpp>
#include <util/misc.hpp>

using namespace geode::prelude;

bool DirectConnectionPopup::setup(ServerSwitcherPopup* parent) {
    this->parent = parent;
    this->setTitle("Direct connection");

    auto rlayout = util::ui::getPopupLayoutAnchored(m_size);

    // address hint
    Build<CCLabelBMFont>::create("Server address", "bigFont.fnt")
        .scale(0.3f)
        .pos(rlayout.fromTop(45.f))
        .parent(m_mainLayer)
        .id("direct-connection-addr-hint"_spr);

    // address input node
    Build<TextInput>::create(POPUP_WIDTH * 0.75f, fmt::format("127.0.0.1:{}", NetworkAddress::DEFAULT_PORT).c_str(), "chatFont.fnt")
        .pos(rlayout.fromTop(65.f))
        .parent(m_mainLayer)
        .id("direct-connection-addr"_spr)
        .store(addressNode);

    addressNode->setFilter(std::string(util::misc::STRING_URL));
    addressNode->setMaxCharCount(64);

    Build<ButtonSprite>::create("Connect", "bigFont.fnt", "GJ_button_01.png", 0.8f)
        .intoMenuItem([this](auto) {
            // this is scary
            static std::regex pattern(R"(^(?!(?:https?):\/\/)(?:(?:[0-9]{1,3}\.){3}[0-9]{1,3}|(?:[a-zA-Z0-9-]+\.)+[a-zA-Z]{2,})(?::\d+)?$)");

            std::string addr = this->addressNode->getString();

            if (addr.empty() || !std::regex_match(addr, pattern)) {
                PopupManager::get().alertFormat(
                    "Error",
                    "Invalid address was passed. It must be an IPv4 address or a domain name with an optional port at the end (like <cy>127.0.0.1:{}</c> or <cy>globed.example.com:{}</c>)",
                    NetworkAddress::DEFAULT_PORT,
                    NetworkAddress::DEFAULT_PORT
                ).showInstant();
                return;
            }

            auto& gsm = GameServerManager::get();
            auto result = gsm.addServer(GameServerManager::STANDALONE_ID, "Server", addr, "unknown");
            if (result.isErr()) {
                PopupManager::get().alertFormat("Error", "Failed to connect to the server: {}", result.unwrapErr()).showInstant();
                return;
            }

            auto& csm = CentralServerManager::get();
            csm.setStandalone();
            csm.recentlySwitched = true;

            gsm.clearAllExcept(GameServerManager::STANDALONE_ID);
            gsm.pendingChanges = true;
            gsm.saveStandalone(addr);

            auto& nm = NetworkManager::get();
            result = nm.connectStandalone();
            if (result.isErr()) {
                PopupManager::get().alert("Error", result.unwrapErr()).showInstant();
            }

            this->onClose(this);
            this->parent->close();
        })
        .id("connect-btn"_spr)
        .intoNewParent(CCMenu::create())
        .id("connect-btn-menu"_spr)
        .pos(rlayout.fromBottom(30.f))
        .parent(m_mainLayer);

    return true;
}

DirectConnectionPopup* DirectConnectionPopup::create(ServerSwitcherPopup* parent) {
    auto ret = new DirectConnectionPopup;
    if (ret->initAnchored(POPUP_WIDTH, POPUP_HEIGHT, parent)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}