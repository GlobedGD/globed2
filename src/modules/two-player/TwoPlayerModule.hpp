#pragma once

#include <globed/core/Module.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <globed/core/data/Messages.hpp>
#include "Events.hpp"

namespace globed {

class VisualPlayer;
class UserListPopup;

class TwoPlayerModule : public SoftModule<TwoPlayerModule> {
public:
    TwoPlayerModule();

    static constexpr inline auto AUTO_ENABLE = AutoEnableMode::Level;
    static inline const ModuleMetadata metadata {
        .id = "globed.two-player-mode",
        .name = "Two Player Mode",
        .author = "Globed",
    };

    bool link(int id, bool player2);
    void unlink(bool silent = false);
    void cancelLink();

    bool isLinked();
    bool isPlayer2();
    bool waitingForLink();
    VisualPlayer* getLinkedPlayerObject(bool player2);

    bool& ignoreNoclip();

    void causeLocalDeath(GJBaseGameLayer* gjbgl);

private:
    std::optional<int> m_linkedPlayer;
    bool m_isPlayer2 = false;
    bool m_ignoreNoclip = false;
    std::optional<int> m_linkAttempt;
    std::shared_ptr<RemotePlayer> m_linkedRp;

    friend SoftModule;

    void onDisabled() override;

    void onJoinLevel(GlobedGJBGL* gjbgl, GJGameLevel* level, bool editor) override;
    void onPlayerDeath(GlobedGJBGL* gjbgl, RemotePlayer* player, const PlayerDeath& death) override;
    void onPlayerRespawn(GlobedGJBGL* gjbgl, RemotePlayer* player) override;
    void onPlayerLeave(GlobedGJBGL* gjbgl, int accountId) override;
    bool shouldSpeedUpNewBest(GlobedGJBGL* gjbgl) override {
        return true;
    }

    bool wantsSyncReset() override {
        return true;
    }

    void onUserlistSetup(cocos2d::CCNode* container, int accountId, bool myself, UserListPopup* popup);
    void onLocalPlayerDeath(GlobedGJBGL* gjbgl, bool real) override;
    void onUpdate(GlobedGJBGL* gjbgl, float dt) override;

    void sendUnlinkEventTo(int id);
    void sendLinkEventTo(int id, bool player2);
    void linkSuccess(int id, bool player2);

    void handleLinkEvent(const TwoPlayerLinkEvent& reader, int playerId);
    void handleUnlinkEvent(const TwoPlayerUnlinkEvent& reader, int playerId);

    void updateFromLinkedPlayer(PlayerObject* local, VisualPlayer* linked);
};

}
