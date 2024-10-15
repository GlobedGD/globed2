#include "menu_layer.hpp"

#include <managers/account.hpp>
#include <managers/central_server.hpp>
#include <managers/error_queues.hpp>
#include <managers/game_server.hpp>
#include <managers/settings.hpp>
#include <managers/friend_list.hpp>
#include <net/manager.hpp>
#include <ui/menu/main/globed_menu_layer.hpp>
#include <ui/menu/servers/server_layer.hpp>
#include <util/cocos.hpp>
#include <util/misc.hpp>
#include <util/net.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

static bool softDisabled = false;
static bool fallbackMenuButton = false;
static bool firstEntry = true;

constexpr auto RESOURCE_DUMMY = "dummy-icon1.png"_spr;

static void checkResources() {
    if (GlobedSettings::get().launchArgs().skipResourceCheck) {
        return;
    }

    if (!util::cocos::isValidSprite(CCSprite::createWithSpriteFrameName(RESOURCE_DUMMY))) {
        log::warn("Failed to find {}, disabling the mod", RESOURCE_DUMMY);
        softDisabled = true;
        return;
    }

    if (!util::cocos::isValidSprite(CCSprite::createWithSpriteFrameName("menuicon.png"_spr))) {
        log::warn("Failed to find menuicon.png, fallback menu button enabled");
        softDisabled = true;
        fallbackMenuButton = true;
        return;
    }
}

bool HookedMenuLayer::init() {
    if (!MenuLayer::init()) return false;

    if (firstEntry) {
        checkResources();
        firstEntry = false;
    }

    if (!softDisabled) {
        // auto connect
        util::misc::callOnce("menu-layer-init-autoconnect", []{
            if (!GlobedSettings::get().globed.autoconnect) return;

            auto& csm = CentralServerManager::get();
            auto& gsm = GameServerManager::get();
            auto& am = GlobedAccountManager::get();

            auto lastId = gsm.loadLastConnected();
            if (lastId.empty()) return;

            am.autoInitialize();

            if (csm.standalone()) {
                auto result = NetworkManager::get().connectStandalone();
                if (result.isErr()) {
                    ErrorQueues::get().warn(fmt::format("Failed to connect: {}", result.unwrapErr()));
                }
            } else {
                auto cacheResult = gsm.loadFromCache();
                if (cacheResult.isErr()) {
                    ErrorQueues::get().debugWarn(fmt::format("failed to autoconnect: {}", cacheResult.unwrapErr()));
                    return;
                }

                auto lastServer = gsm.getServer(lastId);

                if (!lastServer.has_value()) {
                    ErrorQueues::get().debugWarn("failed to autoconnect, game server not found");
                    return;
                };

                if (!am.hasAuthKey()) {
                    // no authkey, don't autoconnect
                    return;
                }

                am.requestAuthToken([lastServer] {
                    auto result = NetworkManager::get().connect(lastServer.value());
                    if (result.isErr()) {
                        ErrorQueues::get().warn(fmt::format("Failed to connect: {}", result.unwrapErr()));
                    }
                });
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
        if (fallbackMenuButton) {
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
    if (softDisabled) {
        geode::createQuickPopup(
            "Globed Error",
            "<cy>Outdated resources</c> were detected. The mod has been <cr>disabled</c> to prevent crashes.\n\nIf you have any <cg>texture packs</c>, or mods that change textures, please try <cy>disabling</c> them.",
            "Dismiss", "More info", [](auto, bool moreInfo) {
                if (!moreInfo) return;

                bool texturePack = [] {
                    auto hasLdr = Loader::get()->isModLoaded("geode.texture-loader");
                    if (!hasLdr) return false;

                    // check if filename of a globedsheet1.png is overriden
                    auto p = CCFileUtils::get()->fullPathForFilename("globedsheet1.png"_spr, false);
                    if (p.empty()) {
                        log::error("Failed to find globedsheet1.png");
                        return false;
                    }

                    if (std::filesystem::path(std::string(p)).parent_path() != Mod::get()->getResourcesDir()) {
                        return true;
                    }

                    return false;
                }();
                bool darkmode = Loader::get()->isModLoaded("bitz.darkmode_v4");

                auto impostorFolderLoc = dirs::getGameDir() / "Resources" / "dankmeme.globed2";
                std::error_code ec{};
                bool impostorFolder = std::filesystem::exists(impostorFolderLoc) && ec == std::error_code{};

                if (impostorFolder) {
                    geode::createQuickPopup(
                        "Note",
                        fmt::format("This is likely caused by a <cr>non-geode</c> texture pack (found in <cy>{}</c>). Automatically <cy>delete</c> the directory?", impostorFolderLoc),
                        "No", "Yes",
                        [impostorFolderLoc](auto, bool delete_) {
                            if (!delete_) {
                                FLAlertLayer::create("Note", "Not deleting, resolve the issue manually.", "Ok")->show();
                                return;
                            }

                            std::error_code ec;
                            std::filesystem::remove_all(impostorFolderLoc, ec);

                            if (ec == std::error_code{}) {
                                geode::createQuickPopup("Success", "Successfully <cg>deleted</c> the folder. Restart the game now?", "Cancel", "Restart", [](auto, bool restart) {
                                    if (restart) {
                                        utils::game::restart();
                                    }
                                });
                            } else {
                                FLAlertLayer::create(
                                    "Error",
                                    fmt::format("Failed to delete the directory: <cy>{}</c>.\n\nPlease delete it manually.", ec.message()),
                                    "Ok"
                                )->show();
                            }
                        }
                    );
                } else if (texturePack || darkmode) {
                    FLAlertLayer::create(
                        "Note",
                        fmt::format("Globed texture pack detected: {}</c>\nDarkMode v4 enabled: {}</c>\n\nPlease try to <cr>disable</c> these and see if the issue is resolved after restarting.", texturePack ? "<cg>yes" : "<cr>no", darkmode ? "<cg>yes" : "<cr>no"),
                        "Ok"
                    )->show();
                } else {
                    geode::createQuickPopup(
                        "Error",
                        "Failed to determine the root cause of the issue. Please create a <cy>bug report</c> on our GitHub page.",
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
        FLAlertLayer::create("Notice", "You need to be signed into a <cg>Geometry Dash account</c> in order to play online.", "Ok")->show();
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