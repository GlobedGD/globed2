#pragma once
#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <globed/config.hpp>
#include <globed/core/game/RemotePlayer.hpp>
#include <globed/core/game/GameEvents.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <globed/core/data/Messages.hpp>
#include <globed/util/BoolExt.hpp>
#include <globed/util/Interval.hpp>
#include <ui/game/VoiceOverlay.hpp>
#include <ui/game/PingOverlay.hpp>
#include <ui/game/EmoteBubble.hpp>
#include <ui/game/ProfilerOverlay.hpp>
#include <ui/misc/NameLabel.hpp>
#include <core/game/Interpolator.hpp>
#include <core/game/SpeedTracker.hpp>

namespace globed {

struct CameraDirection {
    cocos2d::CCPoint vector;
    float angle;
};

struct GameCameraState {
    cocos2d::CCPoint cameraOrigin;
    cocos2d::CCPoint visibleOrigin;
    cocos2d::CCSize visibleCoverage;
    float zoom;

    inline cocos2d::CCSize cameraCoverage() const {
        return visibleCoverage / std::abs(zoom);
    }

    inline cocos2d::CCPoint cameraCenter() const {
        return cameraOrigin + this->cameraCoverage() / 2.f;
    }
};

struct GLOBED_MODIFY_ATTR GlobedGJBGL : geode::Modify<GlobedGJBGL, GJBaseGameLayer> {
    struct Fields {
        ~Fields();
        void cleanup();

        bool m_active = false;
        bool m_cleanedUp = false;
        bool m_editor = false;
        bool m_quitting = false;
        bool m_throttleUpdates = false;
        bool m_disallowThrottle = false;
        float m_periodicalDelta = 0.f;

        float m_timeCounter = 0.0f;
        float m_lastServerUpdate = 0.0f;
        float m_lastTeamRefresh = 0.0f;
        Interval m_sendInterval;
        Interval m_sendThrottledInterval;
        Interval m_audioInterval;
        uint32_t m_totalSentPackets = 0;
        Interpolator m_interpolator;
        VectorSpeedTracker m_cameraTracker;
        std::unordered_map<int, std::shared_ptr<RemotePlayer>> m_players;
        std::shared_ptr<RemotePlayer> m_ghost; // player that always follows the local player
        std::vector<int> m_unknownPlayers;
        float m_lastDataRequest = 0.f;
        MessageListener<msg::LevelDataMessage> m_levelDataListener;
        MessageListener<msg::LevelMetaMessage> m_levelMetaListener;
        MessageListener<msg::VoiceBroadcastMessage> m_voiceListener;
        MessageListener<msg::QuickChatBroadcastMessage> m_quickChatListener;
        MessageListener<msg::ChatNotPermittedMessage> m_mutedListener;
        MessageListener<msg::JoinSessionFailedMessage> m_joinFailedListener;
        MessageListener<msg::DiscordLinkStateMessage> m_discordStateListener;
        geode::ListenerHandle m_displayDataListener;

        uint8_t m_deathCount = 0;
        bool m_lastLocalDeathReal = false;
        bool m_isFakingDeath = false;
        bool m_manualReset = false;
        bool m_safeMode = false;
        bool m_permanentSafeMode = false;
        bool m_playersHidden = false;
        bool m_isVoiceProximity = false;
        bool m_noGlobalCulling = false;
        bool m_sendExtData = false;
        bool m_knownServerMuted = false;
        bool m_knownNotLinked = false;
        bool m_showedMutedAlert = false;
        bool m_deafened = false;
        bool m_showingNoticeAlert = false;
        bool m_spectating = false;
        BoolExt m_didJustJump1, m_didJustJump2;

        CCNode* m_playerNode = nullptr;
        GlobedGJBGL* m_self = nullptr;
        Ref<CCNode> m_progressBarContainer;
        Ref<VoiceOverlay> m_voiceOverlay;
        Ref<ProfilerOverlay> m_profilerOverlay;
        Ref<PingOverlay> m_pingOverlay;
        Ref<CCSprite> m_noticeAlert;
        geode::WeakRef<PlayerObject> m_cameraFollows;
    };

    // Setup functions
    void setupPreInit(GJGameLevel* level, bool editor);
    void setupPostInit();
    void onQuit();
    /// Necessary setup that runs always, even if multiplayer is not active.
    void setupNecessary();
    /// Preload necessary assets if they were not loaded before.
    void setupAssetLoading();
    /// Setup audio
    void setupAudio();
    /// Setup UI
    void setupUi();
    /// Setup message listeners
    void setupListeners();
    /// Setup keybinds
    void setupKeybinds();

    // Hooks
    $override
    bool init(GJGameLevel* level);

    $override
    void updateCamera(float dt);

    void onEnterHook();

    // Schedules
    void selPreUpdate(float dt);
    void selPostUpdate(float dt);
    void selPeriodicalUpdate(float dt);

    // Misc
    PlayerState getPlayerState();
    bool isPaused(bool checkCurrent = true);
    bool isEditor();
    bool isSpectating();
    bool isCurrentPlayLayer();
    bool isManuallyResetting();
    bool isSafeMode();
    bool isQuitting();
    void handlePlayerJoin(int playerId);
    void handlePlayerLeave(int playerId, bool removeFromMap = true);
    void handleLocalPlayerDeath(PlayerObject*);
    void setPermanentSafeMode();
    /// Forces update events to be sent exactly 30 times a second (or whatever the server tickrate is).
    /// By default, the send rate is throttled if no other players are on the level.
    void setDisallowThrottleUpdates();
    void setSpectating(bool spectate);
    void sendPlayerData(const PlayerState& state);
    /// Kills the local player, by default the death will not be counted as 'real'.
    /// If this is unwanted, pass `false`
    void killLocalPlayer(bool fake = true);
    /// Cancel automatic respawn after death, to respawn call causeLocalRespawn()
    void cancelLocalRespawn();
    /// Essentially calls resetLevel (restarting the level), but in a less breaking manner
    void causeLocalRespawn(bool full = false);

    void resetSafeMode();
    void enableSafeMode();

    void pausedUpdate(float dt);

    PlayLayer* asPlayLayer();

    // Functions for the outside :tm:
    static GlobedGJBGL* get(GJBaseGameLayer* base = nullptr);
    static GlobedGJBGL* getActive(GJBaseGameLayer* base = nullptr);

    bool active();
    CameraDirection getCameraDirection();
    GameCameraState getCameraState();
    std::shared_ptr<RemotePlayer> getPlayer(int playerId);
    void recordPlayerJump(bool p1);
    bool shouldLetMessageThrough(int playerId);
    bool isSpeaking(int playerId);
    void setNoticeAlertActive(bool active);
    bool isUnableToSpeak();
    void maybeShowVCAlert(msg::ChatNotPermittedReason reason);
    void updateLocalIcons(std::optional<PlayerIconData> data);

    void toggleCullingEnabled(bool culling);
    void toggleExtendedData(bool extended);
    void toggleHidePlayers();
    void toggleDeafen();
    void resumeVoiceRecording();
    void pauseVoiceRecording();

    void setCameraFollowPlayer(PlayerObject* player);

    bool playSelfEmote(uint32_t id);
    bool playSelfFavoriteEmote(uint32_t which);

private:
    void onLevelDataReceived(const msg::LevelDataMessage& message);
    void onLevelMetaReceived(const msg::LevelMetaMessage& message);
    void onVoiceDataReceived(msg::VoiceBroadcastMessage& message);
    void onQuickChatReceived(int accountId, uint32_t quickChatId);
    void onJoinSessionFailed(const msg::JoinSessionFailedMessage& message);
    void onDisplayDataRefreshed(const DisplayDataRefreshedEvent& ev);

    void cleanupGlobedAdditions();
    void fixProgressBar(float percent);
};

void setPlayerHidden(PlayerObject* obj, bool hidden);
void setPlayerHidden(RemotePlayer* obj, bool hidden);

}