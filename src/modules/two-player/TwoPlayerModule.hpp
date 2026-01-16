#pragma once

#include <globed/core/ModuleCrtp.hpp>
#include <globed/core/data/Messages.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <qunet/buffers/ByteReader.hpp>

namespace globed {

class VisualPlayer;

class TwoPlayerModule : public ModuleCrtpBase<TwoPlayerModule> {
public:
    TwoPlayerModule();

    void onModuleInit();

    virtual std::string_view name() const override
    {
        return "Two Player Mode";
    }

    virtual std::string_view id() const override
    {
        return "globed.two-player-mode";
    }

    virtual std::string_view author() const override
    {
        return "Globed";
    }

    virtual std::string_view description() const override
    {
        return "";
    }

    bool link(int id, bool player2);
    void unlink(bool silent = false);
    void cancelLink();

    bool isLinked();
    bool isPlayer2();
    bool waitingForLink();
    VisualPlayer *getLinkedPlayerObject(bool player2);

    bool &ignoreNoclip();

    void causeLocalDeath(GJBaseGameLayer *gjbgl);

private:
    std::optional<int> m_linkedPlayer;
    bool m_isPlayer2 = false;
    bool m_ignoreNoclip = false;
    std::optional<int> m_linkAttempt;
    std::optional<MessageListenerImpl<msg::LevelDataMessage> *> m_listener;
    std::shared_ptr<RemotePlayer> m_linkedRp;

    geode::Result<> onDisabled() override;

    void onJoinLevel(GlobedGJBGL *gjbgl, GJGameLevel *level, bool editor) override;
    void onPlayerDeath(GlobedGJBGL *gjbgl, RemotePlayer *player, const PlayerDeath &death) override;
    void onUserlistSetup(cocos2d::CCNode *container, int accountId, bool myself, UserListPopup *popup) override;
    void onPlayerLeave(GlobedGJBGL *gjbgl, int accountId) override;
    bool shouldSpeedUpNewBest(GlobedGJBGL *gjbgl) override
    {
        return true;
    }

    void sendUnlinkEventTo(int id);
    void sendLinkEventTo(int id, bool player2);
    void linkSuccess(int id, bool player2);

    void handleEvent(const InEvent &event);
    void handleLinkEvent(const TwoPlayerLinkRequestEvent &reader);
    void handleUnlinkEvent(const TwoPlayerUnlinkEvent &reader);
};

} // namespace globed
