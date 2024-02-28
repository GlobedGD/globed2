#include "server_list_cell.hpp"

#include <Geode/utils/web.hpp>
#include <net/network_manager.hpp>
#include <managers/account.hpp>
#include <managers/game_server.hpp>
#include <managers/central_server.hpp>
#include <managers/error_queues.hpp>
#include <managers/settings.hpp>
#include <util/net.hpp>
#include <util/time.hpp>
#include <util/format.hpp>

using namespace geode::prelude;

bool ServerListCell::init(const GameServer& gsview, bool active) {
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

void ServerListCell::updateWith(const GameServer& gsview, bool active) {
    bool btnChanged = (active != this->active);

    this->gsview = gsview;
    this->active = active;

    labelName->setString(gsview.name.c_str());
    labelName->limitLabelWidth(205.f, 0.7f, 0.1f);

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
                    auto& settings = GlobedSettings::get();
                    if (!settings.flags.seenVoiceChatPTTNotice) {
                        settings.flags.seenVoiceChatPTTNotice = true;
                        settings.save();

                        FLAlertLayer::create("Notice", "Be advised that Globed has voice chat (it can be disabled in settings if you wish). You can hold V to talk to other people.", "Ok")->show();
                        return;
                    }

                    auto& csm = CentralServerManager::get();

                    if (csm.standalone()) {
                        GLOBED_RESULT_ERRC(NetworkManager::get().connectStandalone());
                    } else {
                        // check if the token is here, otherwise request it
                        auto& am = GlobedAccountManager::get();

                        auto hasToken = !am.authToken.lock()->empty();

                        if (hasToken) {
                            GLOBED_RESULT_ERRC(NetworkManager::get().connectWithView(this->gsview));
                        } else {
                            this->requestTokenAndConnect();
                        }
                    }
                }
            })
            .anchorPoint(1.0f, 0.5f)
            .pos(34.f, 0.f)
            .parent(btnMenu)
            .store(btnConnect);
        btnConnect->m_scaleMultiplier = 1.1f;
    }
    
}

void ServerListCell::requestTokenAndConnect() {
    auto& am = GlobedAccountManager::get();
    auto& csm = CentralServerManager::get();

    auto result = am.generateAuthCode();
    if (result.isErr()) {
        // invalid authkey? clear it so the user can relog. happens if user changes their password
        ErrorQueues::get().error(fmt::format(
            "Failed to generate authcode! Please try to login and connect again.\n\nReason for the error: <cy>{}</c>",
            result.unwrapErr()
        ));
        am.clearAuthKey();
        return;
    }

    std::string authcode = result.unwrap();

    auto gdData = am.gdData.lock();

    auto url = fmt::format(
        "{}/totplogin?aid={}&uid={}&aname={}&code={}",
        csm.getActive()->url,
        gdData->accountId,
        gdData->userId,
        util::format::urlEncode(gdData->accountName),
        authcode
    );

    am.requestAuthToken(csm.getActive()->url, gdData->accountId, gdData->userId, gdData->accountName, authcode, [gsview = this->gsview]{
        GLOBED_RESULT_ERRC(NetworkManager::get().connectWithView(gsview));
    });
}

ServerListCell* ServerListCell::create(const GameServer& gsview, bool active) {
    auto ret = new ServerListCell;
    if (ret->init(gsview, active)) {
        return ret;
    }

    delete ret;
    return nullptr;
}