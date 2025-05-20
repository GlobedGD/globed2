#pragma once

#include <defs/geode.hpp>
#include <managers/web.hpp>
#include <managers/game_server.hpp>
#include <managers/popup.hpp>

#include <argon/argon.hpp>
#include <Geode/ui/Popup.hpp>

class ConnectionPopup : public geode::Popup<GameServer>, public UploadMessageDelegate, public MessageListDelegate {
public:
    static constexpr float POPUP_WIDTH = 200.f;
    static constexpr float POPUP_HEIGHT = 80.f;
    static ConnectionPopup* create(GameServer server);

private:
    GameServer m_server;
    cocos2d::CCLabelBMFont* m_statusLabel;
    WebRequestManager::Listener createListener;
    WebRequestManager::Listener finishListener;

    std::string storedAuthcode;
    std::string storedChToken;
    int storedAccountId;
    bool isSecureMode = false;
    bool m_doClose = false;


    enum class State {
        None,
        ArgonAuth,
        RequestedChallenge,
        UploadingChallenge,
        VerifyingChallenge,
        CreatingSessionToken,

        AttemptingConnection,
        RelayEstablishing,
        Establishing,
        Done
    };

    State m_state = State::None;
    argon::AuthProgress m_argonState;

    void forceClose();
    void onClose(cocos2d::CCObject*) override;

    template <typename... Args>
    void onFailure(fmt::format_string<Args...> fmt, Args&&... args) {
        this->forceClose();
        auto p = PopupManager::get().alertFormat("Error", fmt, std::forward<Args>(args)...);
        p.showInstant();
    }

    bool setup(GameServer server) override;
    void update(float dt) override;
    void updateState(State s);
    void startStandalone();
    void startCentral();
    void onArgonFailure(const std::string& error);
    void onAbruptDisconnect();

    void requestTokenAndConnect();

    /* manual challenge stuff */

    void challengeStart();
    void challengeStartCallback(typename WebRequestManager::Event* event);
    void onChallengeCreated(int accountId, std::string_view chtoken, std::string_view pubkey, bool secureMode);
    void onChallengeCompleted(std::string_view authcode);
    void onDelayedChallengeCompleted();
    void challengeCompleteCallback(typename WebRequestManager::Event* event);

    void tryCheckMessageCount();

    void uploadMessageFinished(int) override;
    void uploadMessageFailed(int) override;

    void loadMessagesFinished(cocos2d::CCArray* p0, char const* p1) override;
    void loadMessagesFailed(char const* p0, GJErrorCode p1) override;
};
