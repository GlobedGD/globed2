#pragma once
#include <defs/geode.hpp>

#include <Geode/modify/GJBaseGameLayer.hpp>

#include <data/types/room.hpp>
#include <game/interpolator.hpp>
#include <game/player_store.hpp>
#include <net/network_manager.hpp>
#include <ui/game/player/remote_player.hpp>
#include <ui/game/overlay/overlay.hpp>
#include <ui/game/progress/progress_icon.hpp>
#include <ui/game/progress/progress_arrow.hpp>
#include <ui/game/voice_overlay/overlay.hpp>

float adjustLerpTimeDelta(float dt);

class $modify(GlobedGJBGL, GJBaseGameLayer) {
    // setup stuff
    bool globedReady = false;
    bool setupWasCompleted = false;
    uint32_t configuredTps = 0;

    // in game stuff
    bool deafened = false;
    bool isVoiceProximity = false;
    uint32_t totalSentPackets = 0;
    float timeCounter = 0.f;
    float lastServerUpdate = 0.f;
    std::shared_ptr<PlayerInterpolator> interpolator;
    std::shared_ptr<PlayerStore> playerStore;
    RoomSettings roomSettings;
    bool progressForciblyDisabled = false; // affected by room settings, forces safe mode
    bool forcedPlatformer = false;
    bool shouldStopProgress = false;
    GameCameraState camState;

    std::optional<SpiderTeleportData> spiderTp1, spiderTp2;
    bool didJustJumpp1 = false, didJustJumpp2 = false;
    bool isCurrentlyDead = false;
    float lastDeathTimestamp = 0.f;

    // ui elements
    GlobedOverlay* overlay = nullptr;
    std::unordered_map<int, RemotePlayer*> players;
    Ref<PlayerProgressIcon> selfProgressIcon = nullptr;
    Ref<CCNode> progressBarWrapper = nullptr;
    Ref<PlayerStatusIcons> selfStatusIcons = nullptr;
    Ref<GlobedVoiceOverlay> voiceOverlay = nullptr;
    Ref<cocos2d::CCLabelBMFont> ownNameLabel = nullptr;
    Ref<cocos2d::CCLabelBMFont> ownNameLabel2 = nullptr;

    // speedhack detection
    float lastKnownTimeScale = 1.0f;
    std::unordered_map<int, util::time::time_point> lastSentPacket;

    $override
    bool init();

    $override
    int checkCollisions(PlayerObject* player, float dt, bool p2);

    $override
    void loadLevelSettings();

    // vmt hook
    void onEnterHook();

    // Convenience getter for static cast to GlobedGJBGL
    static GlobedGJBGL* get();

    /* setup stuff to make init() cleaner */
    // all are ran in this order.

    void setupPreInit(GJGameLevel* level);
    void setupAll();
    void setupBare();
    void setupDeferredAssetPreloading();
    void setupAudio();
    void setupPacketListeners();
    void setupCustomKeybinds();
    void setupMisc();
    void setupUpdate();
    void setupUi();

    // runs 0.25 seconds after PlayLayer::init
    void postInitActions(float);

    /* selectors */

    // selSendPlayerData - runs tps (default 30) times per second
    void selSendPlayerData(float);

    // selSendPlayerMetadata - runs every 5 seconds
    void selSendPlayerMetadata(float);

    // selPeriodicalUpdate - runs 4 times a second, does various stuff
    void selPeriodicalUpdate(float);

    // selUpdate - runs every frame, increments the non-decreasing time counter, interpolates and updates players
    void selUpdate(float dt);

    // setlUpdateEstimators - runs 30 times a second, updates volume estimators
    void selUpdateEstimators(float);

    /* player related functions */

    SpecificIconData gatherSpecificIconData(PlayerObject* player);
    PlayerData gatherPlayerData();
    PlayerMetadata gatherPlayerMetadata();

    bool shouldLetMessageThrough(int playerId);
    void updateProximityVolume(int playerId);

    void handlePlayerJoin(int playerId);
    void handlePlayerLeave(int playerId);

    /* misc */

    bool established();
    bool isCurrentPlayLayer();
    bool isPaused(bool checkCurrent = true);
    bool isEditor();

    bool isSafeMode();
    void toggleSafeMode(bool enabled);

    void onQuitActions();

    // runs every frame while paused
    void pausedUpdate(float dt);

    // With speedhack enabled, all scheduled selectors will run more often than they are supposed to.
    // This means, if you turn up speedhack to let's say 100x, you will send 3000 packets per second. That is a big no-no.
    // For naive speedhack implementations, we simply check CCScheduler::getTimeScale and properly reschedule our data sender.
    //
    // For non-naive speedhacks however, ones that don't use CCScheduler::setTimeScale, it is more complicated.
    // We record the time of sending each packet and compare the intervals. If the interval is suspiciously small, we reject the packet.
    // This does result in less smooth experience with non-naive speedhacks however.
    bool accountForSpeedhack(size_t uniqueKey, float cap, float allowance = 0.9f);

    void unscheduleSelectors();
    void rescheduleSelectors();
};
