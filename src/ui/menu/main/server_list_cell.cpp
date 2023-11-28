#include "server_list_cell.hpp"

#include <Geode/utils/web.hpp>
#include <UIBuilder.hpp>

#include <net/network_manager.hpp>
#include <managers/account_manager.hpp>
#include <managers/server_manager.hpp>
#include <managers/error_queues.hpp>
#include <util/net.hpp>

using namespace geode::prelude;

bool ServerListCell::init(const GameServerView& gsview, bool active) {
    if (!CCLayer::init()) return false;

    Build<CCLayer>::create()
        .layout(RowLayout::create()
                    ->setAxisAlignment(AxisAlignment::Start)
                    ->setAutoScale(false)
        )
        .anchorPoint(0.f, 0.5f)
        .pos(10.f, CELL_HEIGHT / 3 * 2)
        .parent(this)
        .store(namePingLayer);

    Build<CCLabelBMFont>::create("", "bigFont.fnt")
        .anchorPoint(0.f, 0.f)
        .scale(0.7f)
        .parent(namePingLayer)
        .store(labelName);

    Build<CCLabelBMFont>::create("", "goldFont.fnt")
        .anchorPoint(0.f, 0.5f)
        .scale(0.4f)
        .parent(namePingLayer)
        .store(labelPing);

    Build<CCLabelBMFont>::create("", "bigFont.fnt")
        .anchorPoint(0.f, 0.5f)
        .pos(10, CELL_HEIGHT * 0.25f)
        .scale(0.25f)
        .parent(this)
        .store(labelExtra);

    Build<CCMenu>::create()
        .pos(CELL_WIDTH - 40.f, CELL_HEIGHT / 2)
        .parent(this)
        .store(btnMenu);

    this->updateWith(gsview, active);

    return true;
}

void ServerListCell::updateWith(const GameServerView& gsview, bool active) {
    bool btnChanged = (active != this->active);

    this->gsview = gsview;
    this->active = active;

    // log::debug("list cell name: {}", gsview.name);

    labelName->setString(gsview.name.c_str());
    labelPing->setString(fmt::format("{} ms", gsview.ping == -1 ? "?" : std::to_string(gsview.ping)).c_str());
    labelExtra->setString(fmt::format("Region: {}, players: {}", gsview.region, gsview.playerCount).c_str());

    labelName->setColor(active ? ACTIVE_COLOR : INACTIVE_COLOR);
    labelExtra->setColor(active ? ACTIVE_COLOR : INACTIVE_COLOR);

    namePingLayer->updateLayout();

    if (btnChanged || !btnConnect) {
        if (btnConnect) {
            btnConnect->removeFromParent();
        }

        Build<ButtonSprite>::create(active ? "Leave" : "Join", "bigFont.fnt", active ? "GJ_button_03.png" : "GJ_button_01.png", 0.8f)
            .intoMenuItem([this, active](auto) {
                if (active) {
                    NetworkManager::get().disconnect(false);
                } else {
                    // check if the token is here
                    auto& sm = GlobedServerManager::get();
                    auto& am = GlobedAccountManager::get();

                    auto hasToken = !am.authToken.lock()->empty();

                    if (hasToken) {
                        NetworkManager::get().connectWithView(this->gsview);
                    } else {
                        this->requestTokenAndConnect();
                    }
                }
            })
            .anchorPoint(1.0f, 0.5f)
            .pos(34.f, 0.f)
            .parent(btnMenu)
            .store(btnConnect);
    }
}

void ServerListCell::requestTokenAndConnect() {
    auto& am = GlobedAccountManager::get();
    auto& sm = GlobedServerManager::get();

    std::string authcode;
    try {
        authcode = am.generateAuthCode();
    } catch (const std::exception& e) {
        // invalid authkey? clear it so the user can relog. happens if user changes their password
        ErrorQueues::get().error(fmt::format(
            "Failed to generate authcode! Please try to login and connect again.\n\nReason for the error: <cy>{}</c>",
            e.what()
        ));
        am.clearAuthKey();
        return;
    }

    auto gdData = am.gdData.lock();

    auto url = fmt::format(
        "{}/totplogin?aid={}&aname={}&code={}",
        sm.getCentral(),
        gdData->accountId,
        gdData->accountName,
        authcode
    );

    // both callbacks should be safe even if GlobedMenuLayer was closed.
    web::AsyncWebRequest()
        .postRequest()
        .userAgent(util::net::webUserAgent())
        .fetch(url).text()
        .then([&am, &sm, gsview = this->gsview](const std::string& response) {
            *am.authToken.lock() = response;
            NetworkManager::get().connectWithView(gsview);
        })
        .expect([&am](const std::string& error) {
            ErrorQueues::get().error(fmt::format(
                "Failed to generate a session token! Please try to login and connect again.\n\nServer response: <cy>{}</c>",
                error
            ));
            am.clearAuthKey();
        })
        .send();
}