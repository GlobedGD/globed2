#include <globed/core/actions.hpp>
#include <globed/core/data/Messages.hpp>
#include <globed/core/RoomManager.hpp>
#include <globed/core/PopupManager.hpp>
#include <globed/util/gd.hpp>
#include <globed/util/singleton.hpp>
#include <globed/util/FunctionQueue.hpp>
#include <globed/util/CCData.hpp>
#include <core/net/NetworkManagerImpl.hpp>
#include <core/hooks/GJBaseGameLayer.hpp>
#include <core/hooks/GameManager.hpp>
#include <ui/menu/WarpLoadPopup.hpp>
#include <ui/menu/GlobedMenuLayer.hpp>
#include <ui/menu/UserPunishmentPopup.hpp>
#include <ui/admin/ModPanelPopup.hpp>
#include <ui/admin/ModUserPopup.hpp>
#include <ui/admin/ModLoginPopup.hpp>
#include <ui/admin/Common.hpp>
#include <ui/misc/InputPopup.hpp>

using namespace geode::prelude;

namespace globed {

static void forceWarp(WarpContext context) {
    auto defer = [&] {
        auto cx = context;
        FunctionQueue::get().queue([cx] {
            warpToSession(cx);
        });
    };

    auto scene = CCScene::get();
    if (!scene || typeinfo_cast<CCTransitionScene*>(scene)) {
        defer();
        return;
    }

    auto children = scene->getChildrenExt();
    if (children.size() > 0) {
        if (auto pl = typeinfo_cast<PlayLayer*>(children[0])) {
            HookedGameManager::get().setNoopSceneEnum();
            pl->onQuit();
            globed::replaceScene(GlobedMenuLayer::create());

            defer();
            return;
        }
    }

    auto session = context.session;
    log::debug("Warping to session {} (room: {}, level: {})", session.asU64(), session.roomId(), session.levelId());

    auto levelId = session.levelId();
    if (levelId == 0) {
        // level 0 means warp to main menu aka do nothing
        return;
    }

    // set temporary server override
    auto& nm= NetworkManagerImpl::get();
    nm.setTemporaryServerOverride(session.serverId());

    // only open level for global
    bool openLevel = context.source == WarpSource::Global;

    auto classify = globed::classifyLevel(levelId);
    if (classify.kind == GameLevelKind::Main) {
        // main levels, go to that page in LevelSelectLayer
        globed::replaceScene(LevelSelectLayer::scene(levelId - 1));

        // globed::replaceScene(PlayLayer::scene(classify.level, false, false));
    } else if (classify.kind == GameLevelKind::Tower) {
        // tower levels, always go straight to playlayer
        globed::replaceScene(PlayLayer::scene(classify.level, false, false));
    } else {
        // custom levels, show a loading popup
        auto popup = WarpLoadPopup::create(levelId, openLevel, false);
        if (popup) popup->show();
    }
}

static std::string describeServer(std::optional<uint8_t> id) {
    if (!id) return "unassigned";

    auto& nm = NetworkManagerImpl::get();
    auto srv = nm.getGameServer(*id);
    if (!srv) return fmt::format("unknown ({})", *id);

    return fmt::format("{} ({})", srv->name, srv->stringId);
}

void warpToSession(WarpContext context) {
    auto& rm = RoomManager::get();

    // if warped by room owner, ignore if we are owner or not in a follower room
    if (context.source == WarpSource::Room && (rm.isOwner() || !rm.isInFollowerRoom())) {
        return;
    }

    // if warped by clicking on a person who's in a different server, ask if we want to switch servers
    if (context.source == WarpSource::Manual) {
        auto& nm = NetworkManagerImpl::get();
        uint8_t targetServer = context.session.serverId();
        auto currentServer = rm.pickServerId();

        bool showNotice = !globed::flag("core.flags.seen-warp-server-switch");

        if (targetServer != currentServer && showNotice) {
            std::string target = describeServer(targetServer);
            std::string current = describeServer(currentServer);

            globed::confirmPopup(
                "Switch servers",
                fmt::format(
                    "You are trying to join someone who's playing on the server <cg>{}</c>, but your chosen server is <cy>{}</c>. Do you want to temporarily <cj>switch</c> your preferred server? In future, you won't be prompted about this again.",
                    target, current
                ),
                "Cancel", "Yes",
                [context](auto) {
                    globed::swapFlag("core.flags.seen-warp-server-switch");
                    forceWarp(context);
                }
            );

            return;
        }
    } else if (context.source == WarpSource::Room) {
        // if the room owner exited the room or joined the same level, do nothing
        if (context.session.asU64() == 0) {
            return;
        }

        auto gjbgl = GlobedGJBGL::get();
        if (gjbgl && gjbgl->m_level && gjbgl->m_level->m_levelID == context.session.levelId()) {
            return;
        }

        auto ref = PopupManager::get().quickPopup(
            "Warp Request",
            fmt::format("The room owner has joined a level, do you want to follow them?"),
            "No", "Yes",
            [context](auto alert, bool yes) {
                if (!yes) return;
                forceWarp(context);
            }
        );
        ref.setPriority(true);
        ref.showQueue();
        return;
    }

    // force warp!
    forceWarp(context);
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

        waitForMessage<msg::NoticeReplyResultMessage>([](const auto& msg) {
            if (!msg.success) {
                globed::alert("Error", fmt::format("Failed to send notice reply: <cy>{}</c>", msg.error));
            } else {
                globed::toastSuccess("Notice reply sent");
            }
        });

        NetworkManagerImpl::get().sendNoticeReply(senderId, outcome.text);
    });
    popup->show();
}

$on_mod(Loaded) {
    auto& nm = NetworkManagerImpl::get();

    nm.listenGlobal<msg::WarpPlayerMessage>([](const auto& msg) {
        globed::warpToSession(WarpContext{ msg.sessionId, WarpSource::Global });
        return ListenerResult::Stop;
    });

    nm.listenGlobal<msg::CentralLoginOkMessage>([&nm](const auto& msg) {
        // admin login
        auto password = nm.getStoredModPassword();
        if (!password.empty()) {
            nm.sendAdminLogin(password);
        }
    });

    nm.listenGlobal<msg::NoticeMessage>([](const msg::NoticeMessage& msg) {
        if (globed::setting<bool>("core.ui.disable-notices")) {
            log::info("Notice received but notices are disabled, ignoring");
            return;
        }

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
    });

    nm.listenGlobal<msg::BannedMessage>([](const msg::BannedMessage& msg) {
        UserPunishmentPopup::create(msg.reason, msg.expiresAt, true)->show();
    });

    nm.listenGlobal<msg::MutedMessage>([](const msg::MutedMessage& msg) {
        UserPunishmentPopup::create(msg.reason, msg.expiresAt, false)->show();
    });

    nm.listenGlobal<msg::WarnMessage>([](const msg::WarnMessage& msg) {
        PopupManager::get().alert("Server Warning", msg.message, "Ok").showQueue();
    });
}

}