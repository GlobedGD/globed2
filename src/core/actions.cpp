#include <globed/core/actions.hpp>
#include <globed/core/data/Messages.hpp>
#include <globed/core/RoomManager.hpp>
#include <globed/core/PopupManager.hpp>
#include <globed/util/gd.hpp>
#include <globed/util/singleton.hpp>
#include <core/net/NetworkManagerImpl.hpp>
#include <core/hooks/GJBaseGameLayer.hpp>
#include <ui/menu/WarpLoadPopup.hpp>
#include <ui/menu/GlobedMenuLayer.hpp>

using namespace geode::prelude;

namespace globed {

static SessionId g_warpctx;

void warpToSession(SessionId session, bool openLevel) {
    // ignore if we are the room host
    if (RoomManager::get().isOwner()) {
        return;
    }

    g_warpctx = session;

    log::debug("Warping to session {} (room: {}, level: {})", session.asU64(), session.roomId(), session.levelId());

    auto levelId = session.levelId();

    if (levelId == 0) {
        // level 0 means warp to main menu
        GlobedMenuLayer::create()->switchTo();
        return;
    }

    auto classify = globed::classifyLevel(levelId);
    if (classify.kind == GameLevelKind::Main) {
        // main levels, go to that page in LevelSelectLayer if `openLevel` is false
        if (!openLevel) {
            globed::pushScene(LevelSelectLayer::scene(levelId - 1));
        } else {
            globed::pushScene(PlayLayer::scene(classify.level, false, false));
        }
    } else if (classify.kind == GameLevelKind::Tower) {
        // tower levels, always go straight to playlayer
        auto level = GameLevelManager::get()->getMainLevel(levelId, false);
        // TODO: idk if they are the same
        log::debug("level 1: {}, 2: {}", level, classify.level);
        globed::pushScene(PlayLayer::scene(level, false, false));
    } else {
        // custom levels, show a loading popup
        WarpLoadPopup::create(levelId, openLevel)->show();
    }
}

SessionId _getWarpContext() {
    return g_warpctx;
}

void _clearWarpContext() {
    g_warpctx = SessionId{};
}

void warpToLevel(int level, bool openLevel) {
    auto& rm = RoomManager::get();

    if (auto srv = rm.pickServerId()) {
        warpToSession(SessionId::fromParts(
            *srv,
            rm.getRoomId(),
            level
        ), openLevel);
    } else {
        globed::alert("Error", "Warp failed, no game servers available!");
    }
}

void openUserProfile(int accountId, int userId, std::string_view username) {
    bool myself = accountId == cachedSingleton<GJAccountManager>()->m_accountID;

    if (!myself) {
        cachedSingleton<GameLevelManager>()->storeUserName(userId, accountId, gd::string(username));
    }

    ProfilePage::create(accountId, myself)->show();
}

void openUserProfile(const RoomPlayer& player) {
    openUserProfile(player.accountData);
}

void openUserProfile(const PlayerAccountData& player) {
    openUserProfile(player.accountId, player.userId, player.username);
}

$on_mod(Loaded) {
    NetworkManagerImpl::get().listenGlobal<msg::WarpPlayerMessage>([](const auto& msg) {
        globed::warpToSession(msg.sessionId, true);
        return ListenerResult::Stop;
    });
}

}