#include "menu_layer.hpp"

#include <managers/account.hpp>
#include <managers/central_server.hpp>
#include <managers/error_queues.hpp>
#include <managers/game_server.hpp>
#include <managers/settings.hpp>
#include <managers/friend_list.hpp>
#include <net/network_manager.hpp>
#include <ui/menu/main/globed_menu_layer.hpp>
#include <util/misc.hpp>
#include <util/net.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

// TODO: remove in the future
void removeFaultyGauntletLevels();

bool HookedMenuLayer::init() {
    if (!MenuLayer::init()) return false;

    if (GJAccountManager::sharedState()->m_accountID > 0) {
        auto& flm = FriendListManager::get();
        flm.maybeLoad();
        removeFaultyGauntletLevels();
    }

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

            auto result = am.generateAuthCode();
            if (result.isErr()) {
                // invalid authkey? clear it so the user can relog. happens if user changes their password
                ErrorQueues::get().debugWarn(fmt::format(
                    "Failed to generate authcode: {}",
                    result.unwrapErr()
                ));
                am.clearAuthKey();
                return;
            }

            std::string authcode = result.unwrap();

            auto gdData = am.gdData.lock();
            am.requestAuthToken(csm.getActive()->url, gdData->accountId, gdData->userId, gdData->accountName, authcode, [lastServer] {
                auto result = NetworkManager::get().connectWithView(lastServer.value());
                if (result.isErr()) {
                    ErrorQueues::get().warn(fmt::format("Failed to connect: {}", result.unwrapErr()));
                }
            });
        }
    });

    m_fields->btnActive = NetworkManager::get().established();
    this->updateGlobedButton();

    CCScheduler::get()->scheduleSelector(schedule_selector(HookedMenuLayer::maybeUpdateButton), this, 0.25f, false);

    return true;
}

void HookedMenuLayer::updateGlobedButton() {
    if (m_fields->globedBtn) {
        m_fields->globedBtn->removeFromParent();
        m_fields->globedBtn = nullptr;
    }

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

    m_fields->globedBtn = Build<CircleButtonSprite>(CircleButtonSprite::createWithSpriteFrameName(
        "menuicon.png"_spr,
        1.f,
        m_fields->btnActive ? CircleBaseColor::Cyan : CircleBaseColor::Green,
        CircleBaseSize::MediumAlt
        ))
        .intoMenuItem([](auto) {
            auto accountId = GJAccountManager::sharedState()->m_accountID;
            if (accountId <= 0) {
                FLAlertLayer::create("Notice", "You need to be signed into a <cg>Geometry Dash account</c> in order to play online.", "Ok")->show();
                return;
            }

            util::ui::switchToScene(GlobedMenuLayer::create());
        })
        .id("main-menu-button"_spr)
        .parent(parent)
        .collect();

    parent->updateLayout();
}

void HookedMenuLayer::maybeUpdateButton(float) {
    bool authenticated = NetworkManager::get().established();
    if (authenticated != m_fields->btnActive) {
        m_fields->btnActive = authenticated;
        this->updateGlobedButton();
    }
}

void removeFaultyGauntletLevels() {
    static std::set<int> gauntletLevels = {
        // fire
        27732941, 28200611, 27483789, 28225110, 27448202,
        // ice
        20635816, 28151870, 25969464, 24302376, 27399722,
        // poison
        28179535, 29094196, 29071134, 26317634, 12107595,
        // shadow
        26949498, 26095850, 27973097, 27694897, 26070995,
        // lava
        18533341, 28794068, 28127292, 4243988, 28677296,
        // bonus
        28255647, 27929950, 16437345, 28270854, 29394058,
        // chaos
        25886024, 4259126, 26897899, 7485599, 19862531,
        // demon
        18025697, 23189196, 27786218, 27728679, 25706351,
        // time
        40638411, 32614529, 31037168, 40937291, 35165900,
        // crystal
        37188385, 35280911, 37187779, 36301959, 36792656,
        // magic
        37269362, 29416734, 36997718, 39853981, 39853458,
        // spike
        27873500, 34194741, 34851342, 36500807, 39749578,
        // monster
        43908596, 41736289, 42843431, 44063088, 44131636,
        // doom
        38427291, 38514054, 36966088, 38398923, 36745142,
        // death
        44121158, 43923301, 43537990, 33244195, 35418014,
        // castle
        80218929, 95436164, 64302902, 65044525, 66960655,
        // world
        83313115, 83325036, 83302544, 83325083, 81451870,
        // galaxy
        83294687, 83323867, 83320529, 83315343, 83324930,
        // universe
        83323273, 83025300, 83296274, 83256789, 83323659,
        // discord
        89521875, 90475659, 90117883, 88266256, 88775556,
        // split
        90459731, 90475597, 90471222, 90251922, 90475473
    };

    auto* gsm = GameStatsManager::sharedState();
    auto* glm = GameLevelManager::sharedState();

    std::vector<gd::string> toDelete;
    std::vector<std::string> toDeleteNames;

    for (const auto& [key, value] : geode::cocos::CCDictionaryExt<gd::string, cocos2d::CCObject*>(glm->m_gauntletLevels)) {
        auto levelId = std::stoi(key);
        if (!gauntletLevels.contains(levelId)) {
            toDelete.push_back(key);
            toDeleteNames.push_back(static_cast<GJGameLevel*>(value)->m_levelName);
        }
    }

    if (toDeleteNames.empty()) return;

    std::string allLevels = toDeleteNames[0];

    for (size_t i = 1; i < toDeleteNames.size(); i++) {
        allLevels += ", " + toDeleteNames[i];
    }

    log::debug("removing bad gauntlet levels: {}", allLevels);

    for (const auto& del : toDelete) {
        glm->m_gauntletLevels->removeObjectForKey(del);
    }
}