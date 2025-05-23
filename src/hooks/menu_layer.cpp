#include "menu_layer.hpp"

#include <globed/integrity.hpp>
#include <managers/account.hpp>
#include <managers/central_server.hpp>
#include <managers/error_queues.hpp>
#include <managers/game_server.hpp>
#include <managers/settings.hpp>
#include <managers/friend_list.hpp>
#include <managers/popup.hpp>
#include <net/manager.hpp>
#include <ui/menu/main/globed_menu_layer.hpp>
#include <ui/menu/servers/server_layer.hpp>
#include <util/cocos.hpp>
#include <util/misc.hpp>
#include <util/net.hpp>
#include <util/ui.hpp>

#include <argon/argon.hpp>
#include <asp/fs.hpp>

using namespace geode::prelude;

bool HookedMenuLayer::init() {
    if (!MenuLayer::init()) return false;

    // reset integrity check state here
    globed::resetIntegrityCheck();

    if (!globed::hasSeverelyBrokenResources()) {
        // auto connect
        util::misc::callOnce("menu-layer-init-autoconnect", []{
            auto& settings = GlobedSettings::get();

            if (!settings.globed.autoconnect) return;
            if (!settings.flags.seenSignupNoticev3) return;

            auto& csm = CentralServerManager::get();
            auto& gsm = GameServerManager::get();
            auto& am = GlobedAccountManager::get();

            auto lastId = gsm.loadLastConnected();
            if (lastId.empty()) return;

            am.autoInitialize();

            if (csm.standalone()) {
                auto result = NetworkManager::get().connectStandalone();
                if (result.isErr()) {
                    ErrorQueues::get().warn(fmt::format("[Globed] Failed to connect: {}", result.unwrapErr()));
                }
            } else {
                auto cacheResult = csm.initFromCache();
                if (cacheResult.isErr()) {
                    ErrorQueues::get().debugWarn(fmt::format("failed to autoconnect: {}", cacheResult.unwrapErr()));
                    return;
                }

                auto lastServer = gsm.getServer(lastId);

                if (!lastServer.has_value()) {
                    ErrorQueues::get().debugWarn("failed to autoconnect, game server not found");
                    return;
                };

                auto doConnect = [&am, &csm, lastServer] {
                    am.requestAuthToken([lastServer](bool success) {
                        if (!success) {
                            return;
                        }

                        auto result = NetworkManager::get().connect(lastServer.value());

                        if (result.isErr()) {
                            ErrorQueues::get().warn(fmt::format("[Globed] Failed to connect: {}", result.unwrapErr()));
                        }
                    }, csm.activeArgonUrl().has_value());
                };

                bool useArgon = csm.activeHasAuth() && csm.activeArgonUrl().has_value();

                if (!useArgon && !am.hasAuthKey()) {
                    // no authkey, don't autoconnect
                    return;
                } else if (useArgon) {
                    // request the argon token, only then connect
                    (void) argon::setServerUrl(csm.activeArgonUrl().value());
                    argon::setCertVerification(!settings.launchArgs().noSslVerification);
                    (void) argon::startAuth([&am, doConnect](Result<std::string> token) {
                        if (token) {
                            am.storeArgonToken(token.unwrap());
                            doConnect();
                        } else {
                            log::warn("Argon auth failed: {}", token.unwrapErr());
                        }
                    });

                } else {
                    doConnect();
                }
            }
        });
    }

    m_fields->btnActive = NetworkManager::get().established();
    this->updateGlobedButton();

    this->schedule(schedule_selector(HookedMenuLayer::maybeUpdateButton), 0.25f);

    return true;
}

void HookedMenuLayer::updateGlobedButton() {
    CCNode* parent = nullptr;
    CCPoint pos;

    if ((parent = this->getChildByID("bottom-menu"))) {
        pos = CCPoint{0.f, 0.f};
    } else if ((parent = this->getChildByID("side-menu"))) {
        pos = CCPoint{0.f, 0.f};
    } else if ((parent = this->getChildByID("right-side-menu"))) {
        pos = CCPoint{0.f, 0.f};
    }

    if (!parent) {
        log::warn("failed to position the globed button");
        return;
    }

    auto makeSprite = [this]() -> CCNode* {
        if (globed::useFallbackMenuButton()) {
            return CCSprite::createWithSpriteFrameName("GJ_reportBtn_001.png");
        } else {
            return CircleButtonSprite::createWithSpriteFrameName(
                "menuicon.png"_spr,
                1.f,
                m_fields->btnActive ? CircleBaseColor::Cyan : CircleBaseColor::Green,
                CircleBaseSize::MediumAlt
            );
        }
    };

    if (!m_fields->globedBtn) {
        m_fields->globedBtn = Build<CCNode>(makeSprite())
            .intoMenuItem(this, menu_selector(HookedMenuLayer::onGlobedButton))
            .zOrder(5) // force it to be at the end of the layout for consistency
            .id("main-menu-button"_spr)
            .parent(parent)
            .collect();
    } else {
        m_fields->globedBtn->setNormalImage(makeSprite());
    }

    parent->updateLayout();
}

void HookedMenuLayer::maybeUpdateButton(float) {
    bool authenticated = NetworkManager::get().established();
    if (authenticated != m_fields->btnActive) {
        m_fields->btnActive = authenticated;
        this->updateGlobedButton();
    }
}

void HookedMenuLayer::onGlobedButton(CCObject*) {
    if (globed::hasSeverelyBrokenResources()) {
        geode::createQuickPopup(
            "Globed Error",
            "<cy>Outdated resources</c> were detected. The mod has been <cr>disabled</c> to prevent crashes.\n\nIf you have any <cg>texture packs</c>, or mods that change textures, please try <cy>disabling</c> them.",
            "Dismiss", "More info", [](auto, bool moreInfo) {
                if (!moreInfo) return;

                auto report = globed::getIntegrityReport();
                std::string debugData = report.asDebugData();

                log::debug("complete tp debug data: {}", debugData);

                if (report.impostorFolder) {
                    auto popupBody = fmt::format("This is likely caused by a <cr>non-geode</c> texture pack (found in <cy>{}</c>). Automatically <cy>delete</c> the directory?", report.impostorFolder.value());
                    geode::createQuickPopup(
                        "Note",
                        popupBody,
                        "No", "Yes",
                        [report = std::move(report)](auto, bool delete_) {
                            if (!delete_) {
                                PopupManager::get().alert("Note", "Not deleting, resolve the issue manually.").showInstant();
                                return;
                            }

                            auto res = asp::fs::removeDirAll(report.impostorFolder.value());

                            if (res) {
                                geode::createQuickPopup("Success", "Successfully <cg>deleted</c> the folder. Restart the game now?", "Cancel", "Restart", [](auto, bool restart) {
                                    if (restart) {
                                        utils::game::restart();
                                    }
                                });
                            } else {
                                PopupManager::get().alertFormat(
                                    "Error",
                                    "Failed to delete the directory: <cy>{}</c>.\n\nPlease delete it manually.",
                                    res.unwrapErr().message()
                                ).showInstant();
                            }
                        }
                    );
                } else if (report.hasTexturePack || report.darkmodeEnabled) {
                    std::string enabledtxt;

                    if (report.hasTexturePack) {
                        enabledtxt += "<cy>A texture pack</c> that <cj>modifies Globed textures</c> was detected. Please try to <cr>disable</c> this texture pack and wait until the creator updates it.\n";
                    } else if (report.darkmodeEnabled) {
                        enabledtxt += "<cy>DarkMode v4</c> mod was detected, this mod <cj>modifies Globed textures</c>. Please try to <cr>disable</c> it and wait until the creator updates it.\n";
                    }

                    if (report.sheetFilesSeparated) {
                        enabledtxt += "If you are the author of the texture pack, please include the <cg>.plist files</c> of spritesheets in your texture pack to resolve this.\n";
                    }

                    PopupManager::get().alertFormat(
                        "Note",
                        "{}\nDebug data: <cy>{}</c>",
                        enabledtxt, debugData
                    ).showInstant();
                } else {
                    auto latestLog = globed::getLatestLogFile();
                    std::string body = fmt::format("Failed to determine the root cause of the issue (debug data: <cy>{}</c>). Please create a <cy>bug report</c> on our GitHub page, including this log file:\n\n<cy>{}</c>", debugData, latestLog);

                    geode::createQuickPopup(
                        "Error",
                        body,
                        "Dismiss",
                        "Open page",
                        [](auto, bool open) {
                            if (open) {
                                geode::utils::web::openLinkInBrowser(globed::string<"github-issues">());
                            }
                        }
                    );
                }
        });

        return;
    }

    auto accountId = GJAccountManager::sharedState()->m_accountID;
    if (accountId <= 0) {
        PopupManager::get().alert("Notice", "You need to be signed into a <cg>Geometry Dash account</c> in order to play online.").showInstant();
        return;
    }

    auto& settings = GlobedSettings::get();
    if (!settings.flags.seenSignupNoticev3) {
        PopupManager::get().manage(MDPopup::create(
            "Note",
            "To <cg>improve user experience</c>, Globed needs to access your GD account, specifically:\n\n"
            "* Send a <cy>message</c> to a <cj>bot account</c>, to <cg>verify</c> your account. The message will be <cj>automatically deleted</c>.\n"
            "* Send your <cy>friend list</c> to the <cj>Globed server</c>, for features like friend-only invites (and some others) to work."
            " This data is <cg>never stored or logged</c> on the server, and is <cr>deleted</c> once you disconnect.\n\n"
            "If you do <cr>not</c> consent to this, press 'Cancel'.",
            "Cancel", "Ok",
            [this, &settings](bool yea) {
                if (!yea) return;

                settings.flags.seenSignupNoticev3 = true;
                this->onGlobedButton(this);
            }
        )).showInstant();

        return;
    }

    if (NetworkManager::get().established()) {
        util::ui::switchToScene(GlobedMenuLayer::create());
    } else {
        util::ui::switchToScene(GlobedServersLayer::create());
    }
}

#if 0
#include <util/debug.hpp>
void HookedMenuLayer::onMoreGames(cocos2d::CCObject* s) {
    util::debug::Benchmarker bb;
    bb.runAndLog([&] {
        for (size_t i = 0; i < 1 * 1024 * 1024; i++) {
            auto x = typeinfo_cast<CCMenuItemSpriteExtra*>(s);
            if (!x) throw "wow";
        }
    }, "typeinfo");
}
#endif
