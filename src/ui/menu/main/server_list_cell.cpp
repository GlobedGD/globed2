#include "server_list_cell.hpp"

#include <Geode/utils/web.hpp>
#include <net/manager.hpp>
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

                    auto post = [this] {
                        auto& csm = CentralServerManager::get();

                        if (csm.standalone()) {
                            GLOBED_RESULT_ERRC(NetworkManager::get().connectStandalone());
                        } else {
                            // check if the token is here, otherwise request it
                            auto& am = GlobedAccountManager::get();

                            auto hasToken = !am.authToken.lock()->empty();

                            if (hasToken) {
                                GLOBED_RESULT_ERRC(NetworkManager::get().connect(this->gsview));
                            } else {
                                this->requestTokenAndConnect();
                            }
                        }
                    };

                    if (!settings.flags.seenVoiceChatPTTNotice) {
                        settings.flags.seenVoiceChatPTTNotice = true;

                        geode::createQuickPopup("Voice chat", "Do you want to enable <cp>voice chat</c>? This can be changed later in settings.", "Disable", "Enable", [&settings, post](FLAlertLayer*, bool enabled) {
                            settings.communication.voiceEnabled = enabled;
                            post();
#ifndef GLOBED_VOICE_CAN_TALK
                            if (enabled) {
                                // if this is a platform that cannot use the microphone, show an additional popup
                                FLAlertLayer::create("Notice", "Please note that talking is <cr>currently unimplemented</c> on this platform, and you will only be able to hear others. Sorry for the inconvenience.", "Ok")->show();
                            }
#endif
                        });

                        return;
                    }

                    post();
                }
            })
            .anchorPoint(1.0f, 0.5f)
            .pos(34.f, 0.f)
            .scaleMult(1.1f)
            .parent(btnMenu)
            .store(btnConnect);
    }

}

void ServerListCell::requestTokenAndConnect() {
    auto& am = GlobedAccountManager::get();
    auto& csm = CentralServerManager::get();

    am.requestAuthToken(csm.getActive()->url, [gsview = this->gsview]{
        GLOBED_RESULT_ERRC(NetworkManager::get().connect(gsview));
    });
}

ServerListCell* ServerListCell::create(const GameServer& gsview, bool active) {
    auto ret = new ServerListCell;
    if (ret->init(gsview, active)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}