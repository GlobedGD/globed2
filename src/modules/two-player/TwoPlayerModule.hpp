#pragma once

#include <globed/core/ModuleCrtp.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <globed/core/data/Messages.hpp>
#include <qunet/buffers/ByteReader.hpp>

namespace globed {

class VisualPlayer;

class TwoPlayerModule : public ModuleCrtpBase<TwoPlayerModule> {
public:
    TwoPlayerModule();

    void onModuleInit();

    virtual std::string_view name() const override {
        return "Two Player Mode";
    }

    virtual std::string_view id() const override {
        return "globed.two-player-mode";
    }

    virtual std::string_view author() const override {
        return "Globed";
    }

    virtual std::string_view description() const override {
        return "";
    }

    bool link(int id, bool player2);
    void unlink(bool silent = false);
    void cancelLink();

    bool isLinked();
    bool isPlayer2();
    bool waitingForLink();
    VisualPlayer* getLinkedPlayerObject(bool player2);

    bool& ignoreNoclip();

private:
    std::optional<int> m_linkedPlayer;
    bool m_isPlayer2 = false;
    bool m_ignoreNoclip = false;
    std::optional<int> m_linkAttempt;
    std::optional<MessageListenerImpl<msg::LevelDataMessage>*> m_listener;
    RemotePlayer* m_linkedRp = nullptr;

    geode::Result<> onDisabled() override;

    void onJoinLevel(GlobedGJBGL* gjbgl, GJGameLevel* level, bool editor) override;
    void onPlayerDeath(GlobedGJBGL* gjbgl, RemotePlayer* player, const PlayerDeath& death) override;
    void onUserlistSetup(cocos2d::CCNode* container, int accountId, bool myself, UserListPopup* popup) override;
    void onPlayerLeave(GlobedGJBGL* gjbgl, int accountId) override;

    void sendUnlinkEventTo(int id);
    void sendLinkEventTo(int id, bool player2);
    void linkSuccess(int id, bool player2);

    void handleEvent(const Event& event);
    qn::ByteReader::Result<> handleLinkEvent(qn::ByteReader& reader);
    qn::ByteReader::Result<> handleUnlinkEvent(qn::ByteReader& reader);
};

}
