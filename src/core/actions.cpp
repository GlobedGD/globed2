#include <globed/core/actions.hpp>
#include <globed/core/data/Messages.hpp>
#include <globed/core/RoomManager.hpp>
#include <globed/core/PopupManager.hpp>
#include <globed/util/gd.hpp>
#include <globed/util/singleton.hpp>
#include <globed/util/FunctionQueue.hpp>
#include <core/net/NetworkManagerImpl.hpp>
#include <core/hooks/GJBaseGameLayer.hpp>
#include <core/hooks/GameManager.hpp>
#include <ui/menu/WarpLoadPopup.hpp>
#include <ui/menu/GlobedMenuLayer.hpp>
#include <ui/menu/UserPunishmentPopup.hpp>
#include <ui/admin/ModPanelPopup.hpp>
#include <ui/admin/ModUserPopup.hpp>
#include <ui/admin/ModLoginPopup.hpp>
#include <ui/misc/InputPopup.hpp>

using namespace geode::prelude;

namespace globed {

static SessionId g_warpctx;
static std::optional<SessionId> g_awaitingWarp;

void warpToSession(SessionId session, bool openLevel, bool force) {
    auto& rm = RoomManager::get();

    // ignore if we are the room host or not a follower room
    if (!force && (rm.isOwner() || !rm.isInFollowerRoom())) {
        g_awaitingWarp.reset();
        return;
    }

    auto putOnHold = [&] {
        g_awaitingWarp = session;

        FunctionQueue::get().queue([openLevel, force] {
            if (g_awaitingWarp.has_value()) {
                warpToSession(g_awaitingWarp.value(), openLevel, force);
            }
        });
    };

    // delay in case it's a bad time to warp right now
    auto scene = CCScene::get();
    if (typeinfo_cast<CCTransitionScene*>(scene)) {
        // put it on hold
        putOnHold();
        return;
    }

    auto children = scene->getChildrenExt();
    if (children.size() > 0) {
        if (auto pl = typeinfo_cast<PlayLayer*>(children[0])) {
            HookedGameManager::get().setNoopSceneEnum();
            pl->onQuit();
            globed::replaceScene(GlobedMenuLayer::create());

            putOnHold();
            return;
        }
    }

    log::debug("Warping to session {} (room: {}, level: {})", session.asU64(), session.roomId(), session.levelId());

    g_awaitingWarp.reset();
    g_warpctx = session;

    auto levelId = session.levelId();
    if (levelId == 0) {
        // level 0 means warp to main menu aka do nothing
        return;
    }

    auto classify = globed::classifyLevel(levelId);
    if (classify.kind == GameLevelKind::Main) {
        // main levels, go to that page in LevelSelectLayer if `openLevel` is false
        if (!openLevel) {
            globed::replaceScene(LevelSelectLayer::scene(levelId - 1));
        } else {
            globed::replaceScene(PlayLayer::scene(classify.level, false, false));
        }
    } else if (classify.kind == GameLevelKind::Tower) {
        // tower levels, always go straight to playlayer
        globed::replaceScene(PlayLayer::scene(classify.level, false, false));
    } else {
        // custom levels, show a loading popup
        // replace scene if openLevel is true, push scene otherwise
        auto popup = WarpLoadPopup::create(levelId, openLevel, openLevel);
        if (popup) popup->show();
    }
}

void openModPanel(int accountId) {
    if (!NetworkManagerImpl::get().isAuthorizedModerator()) {
        // show a login popup and upon successful login, call this function again
        ModLoginPopup::create([accountId] {
            openModPanel(accountId);
        })->show();

        return;
    }

    if (accountId == 0) {
        ModPanelPopup::create()->show();
    } else {
        ModUserPopup::create(accountId)->show();
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
        cachedSingleton<GameLevelManager>()->storeUserName(userId, accountId, gd::string(username.data(), username.size()));
    }

    ProfilePage::create(accountId, myself)->show();
}

void openUserProfile(const RoomPlayer& player) {
    openUserProfile(player.accountData);
}

void openUserProfile(const PlayerAccountData& player) {
    openUserProfile(player.accountId, player.userId, player.username);
}

static void promptForNoticeReply(int senderId) {
    auto popup = InputPopup::create("chatFont.fnt");
    popup->setMaxCharCount(280);
    popup->setTitle("Enter Reply Text");
    popup->setPlaceholder("Message");
    popup->setCallback([senderId](auto outcome) {
        if (outcome.cancelled) return;
        NetworkManagerImpl::get().sendNoticeReply(senderId, outcome.text);
    });
    popup->show();
}

$on_mod(Loaded) {
    auto& nm = NetworkManagerImpl::get();

    nm.listenGlobal<msg::WarpPlayerMessage>([](const auto& msg) {
        globed::warpToSession(msg.sessionId, true, true);
        return ListenerResult::Stop;
    });

    nm.listenGlobal<msg::CentralLoginOkMessage>([&nm](const auto& msg) {
        // admin login
        auto password = nm.getStoredModPassword();
        if (!password.empty()) {
            nm.sendAdminLogin(password);
        }

        return ListenerResult::Continue;
    });

    nm.listenGlobal<msg::NoticeMessage>([](const msg::NoticeMessage& msg) {
        std::string title;

        if (msg.isReply) {
            title = fmt::format("Reply from {}", msg.senderName);
        } else if (msg.senderId != 0 && !msg.senderName.empty()) {
            title = fmt::format("Globed Notice from {}", msg.senderName);
        } else {
            title = fmt::format("Globed Notice");
        }

        PopupRef popup;

        if (msg.canReply) {
            // if we can reply, show an alert with 2 buttons
            popup = PopupManager::get().quickPopup(title, msg.message, "Ok", "Reply", [sender = msg.senderId](auto, bool reply) {
                if (!reply) return;
                promptForNoticeReply(sender);
            }, 400.f);
        } else {
            // otherwise regular alert
            popup = PopupManager::get().alert(title, msg.message, "Ok", nullptr, 400.f);
        }

        popup.showQueue();

        if (auto title = popup.getInner()->m_mainLayer->getChildByType<CCLabelBMFont>(0)) {
            title->limitLabelWidth(360.f, 0.9f, 0.5f);
        }

        return ListenerResult::Continue;
    });

    nm.listenGlobal<msg::BannedMessage>([](const msg::BannedMessage& msg) {
        UserPunishmentPopup::create(msg.reason, msg.expiresAt, true)->show();
        return ListenerResult::Continue;
    });

    nm.listenGlobal<msg::MutedMessage>([](const msg::MutedMessage& msg) {
        UserPunishmentPopup::create(msg.reason, msg.expiresAt, false)->show();
        return ListenerResult::Continue;
    });

    nm.listenGlobal<msg::WarnMessage>([](const msg::WarnMessage& msg) {
        PopupManager::get().alert("Server Warning", msg.message, "Ok").showQueue();
        return ListenerResult::Continue;
    });
}

}