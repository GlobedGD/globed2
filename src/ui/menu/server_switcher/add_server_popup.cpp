#include "add_server_popup.hpp"

#include <regex>

#include "server_switcher_popup.hpp"
#include "server_test_popup.hpp"
#include <managers/central_server.hpp>
#include <managers/account.hpp>
#include <managers/game_server.hpp>
#include <net/manager.hpp>
#include <util/misc.hpp>

using namespace geode::prelude;

bool AddServerPopup::setup(int modifyingIndex, ServerSwitcherPopup* parent) {
    this->setTitle(modifyingIndex == -1 ? "Add a new server" : "Edit server");
    this->parent = parent;
    this->modifyingIndex = modifyingIndex;

    float popupCenter = CCDirector::get()->getWinSize().width / 2;
    // name hint
    Build<CCLabelBMFont>::create("Server name", "bigFont.fnt")
        .scale(0.3f)
        .pos(popupCenter, POPUP_HEIGHT - 20.f)
        .id("add-server-name-hint"_spr)
        .parent(m_mainLayer);

    // name input node
    Build<InputNode>::create(POPUP_WIDTH * 0.75f, "Server name")
        .pos(popupCenter, POPUP_HEIGHT - 40.f)
        .parent(m_mainLayer)
        .id("add-server-name"_spr)
        .store(nameNode);

    // url hint
    Build<CCLabelBMFont>::create("Server URL", "bigFont.fnt")
        .scale(0.3f)
        .pos(popupCenter, POPUP_HEIGHT - 80.f)
        .id("add-server-url-hint"_spr)
        .parent(m_mainLayer);

    // url input node
    Build<InputNode>::create(POPUP_WIDTH * 0.75f, "Server URL", "chatFont.fnt", std::string(util::misc::STRING_URL), 50)
        .pos(popupCenter, POPUP_HEIGHT - 100.f)
        .parent(m_mainLayer)
        .id("add-server-url"_spr)
        .store(urlNode);

    if (modifyingIndex != -1) {
        auto& csm = CentralServerManager::get();
        auto server = csm.getServer(modifyingIndex);

        nameNode->setString(server.name);
        urlNode->setString(server.url);
    }

    // create button
    Build<ButtonSprite>::create(modifyingIndex == -1 ? "Create" : "Save", "bigFont.fnt", "GJ_button_01.png", 0.8f)
        .intoMenuItem([this](auto) {
            static std::regex serverPattern("^(https?://[^\\s]+[.][^\\s]+)$");

            std::string name = this->nameNode->getString();
            std::string url = this->urlNode->getString();

            if (name.empty() || url.empty()) {
                return;
            }

            if (!std::regex_match(url, serverPattern)) {
                FLAlertLayer::create(
                    "Invalid URL",
                    "The URL provided does not match the needed schema. It must be either a domain name (like <cy>http://example.com</c>) or an IP address (like <cy>http://127.0.0.1:41000</c>)",
                    "Ok"
                )->show();
                return;
            }

            if (url.ends_with('/')) {
                url.pop_back();
            }

            auto& csm = CentralServerManager::get();

            if (this->modifyingIndex == -1 || csm.getServer(this->modifyingIndex).url != url) {
                // we try to connect to the server and see if the versions match
                ServerTestPopup::create(url, this)->show();
                this->retain();
                testedName = name;
                testedUrl = url;
            } else {
                // if we just changed the name of an existing server, simply update it
                testedName = name;
                testedUrl = url;
                this->retain();
                this->onTestSuccess();
            }

        })
        .id("add-server-button"_spr)
        .intoNewParent(CCMenu::create())
        .id("add-server-button-menu"_spr)
        .pos(popupCenter, 55.f)
        .parent(m_mainLayer);

    return true;
}

void AddServerPopup::onTestSuccess() {
    auto& csm = CentralServerManager::get();

    auto newServer = CentralServer {
        .name = testedName,
        .url = testedUrl
    };

    if (modifyingIndex == -1) {
        csm.addServer(newServer);
    } else {
        auto oldServer = csm.getServer(modifyingIndex);

        // if it's the server we are connected to right now, and it's a completely different URL,
        // do essentially the same stuff we do when switching to another server (copied from server_cell.cpp)
        csm.modifyServer(modifyingIndex, newServer);

        if (modifyingIndex == csm.getActiveIndex() && oldServer.url != newServer.url) {
            csm.switchRoutine(modifyingIndex, true);
        }
    }

    this->release();
    this->parent->reloadList();
    this->onClose(this);
}

void AddServerPopup::onTestFailure(const std::string_view message) {
    FLAlertLayer::create("Globed error", message.data(), "Ok")->show();
    this->release();
    this->onClose(this);
}

AddServerPopup* AddServerPopup::create(int modifyingIndex, ServerSwitcherPopup* parent) {
    auto ret = new AddServerPopup;
    if (ret->init(POPUP_WIDTH, POPUP_HEIGHT, modifyingIndex, parent)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}