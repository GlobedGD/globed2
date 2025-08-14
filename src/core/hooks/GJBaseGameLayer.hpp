#pragma once
#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <globed/config.hpp>
#include <globed/core/game/RemotePlayer.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <globed/core/data/Messages.hpp>
#include <core/game/Interpolator.hpp>

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
};

struct GLOBED_MODIFY_ATTR GlobedGJBGL : geode::Modify<GlobedGJBGL, GJBaseGameLayer> {
    struct Fields {
        bool m_active = false;
        bool m_editor = false;
        bool m_didSchedule = false;
        float m_sendDataInterval = 0.0f;

        float m_timeCounter = 0.0f;
        float m_lastServerUpdate = 0.0f;
        float m_lastDataSend = 0.0f;
        float m_lastTeamRefresh = 0.0f;
        uint32_t m_totalSentPackets = 0;
        Interpolator m_interpolator;
        float m_lastP1XPosition = 0.f;
        std::unordered_map<int, std::unique_ptr<RemotePlayer>> m_players;
        std::vector<int> m_unknownPlayers;
        float m_lastDataRequest = 0.f;
        std::optional<MessageListener<msg::LevelDataMessage>> m_levelDataListener;

        uint8_t m_deathCount = 0;
        bool m_lastLocalDeathReal = false;
        bool m_isFakingDeath = false;
        bool m_manualReset = false;
        bool m_safeMode = false;
        bool m_permanentSafeMode = false;

        CCNode* m_playerNode = nullptr;
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
    /// Setup the update loop and other schedules
    void setupUpdateLoop();
    /// Setup UI
    void setupUi();
    /// Setup message listeners
    void setupListeners();

    // Hooks
    $override
    bool init(GJGameLevel* level);

    // Schedules
    void selUpdateProxy(float dt);
    void selUpdate(float dt);
    void selPostInitActions(float dt);
    void selSendPlayerData(float dt);

    // Misc
    PlayerState getPlayerState();
    bool isPaused(bool checkCurrent = true);
    bool isEditor();
    bool isCurrentPlayLayer();
    bool isManuallyResetting();
    bool isSafeMode();
    void handlePlayerJoin(int playerId);
    void handlePlayerLeave(int playerId);
    void handleLocalPlayerDeath(PlayerObject*);
    void setPermanentSafeMode();
    /// Kills the local player, by default the death will not be counted as 'real'.
    /// If this is unwanted, pass `false`
    void killLocalPlayer(bool fake = true);

    void resetSafeMode();
    void toggleSafeMode();

    void pausedUpdate(float dt);

    // Functions for the outside :tm:
    static GlobedGJBGL* get(GJBaseGameLayer* base = nullptr);
    bool active();
    CameraDirection getCameraDirection();
    GameCameraState getCameraState();
    RemotePlayer* getPlayer(int playerId);

private:
    void onLevelDataReceived(const msg::LevelDataMessage& message);
};

}