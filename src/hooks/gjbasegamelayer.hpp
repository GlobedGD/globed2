#pragma once
#include <defs/geode.hpp>

#include <Geode/modify/GJBaseGameLayer.hpp>

#include <globed.hpp>

#include <data/types/room.hpp>
#include <game/interpolator.hpp>
#include <game/player_store.hpp>
#include <game/module/base.hpp>
#include <managers/hook.hpp>
#include <net/manager.hpp>
#include <ui/game/player/remote_player.hpp>
#include <ui/game/overlay/overlay.hpp>
#include <ui/game/progress/progress_icon.hpp>
#include <ui/game/progress/progress_arrow.hpp>
#include <ui/game/voice_overlay/overlay.hpp>
#include <util/time.hpp>

#include <asp/time/Instant.hpp>

float adjustLerpTimeDelta(float dt);

struct GLOBED_DLL GlobedGJBGL : geode::Modify<GlobedGJBGL, GJBaseGameLayer> {
    struct Fields {
        // setup stuff
        bool globedReady = false;
        bool setupWasCompleted = false;
        bool isEditor = false;
        uint32_t configuredTps = 0;

        // in game stuff
        bool deafened = false;
        bool isVoiceProximity = false;
        uint32_t totalSentPackets = 0;
        float timeCounter = 0.f;
        float lastServerUpdate = 0.f;
        std::unique_ptr<PlayerInterpolator> interpolator;
        std::unique_ptr<PlayerStore> playerStore;
        RoomSettings roomSettings;
        bool arePlayersHidden = false;

        std::vector<std::unique_ptr<BaseGameplayModule>> modules;

        bool isManuallyResettingLevel = false;

        bool progressForciblyDisabled = false; // affected by room settings, forces safe mode
        bool forcedPlatformer = false;
        bool shouldStopProgress = false;
        bool quitting = false;
        bool shouldRequestMeta = false;
        bool isFakingDeath = false;
        GameCameraState camState;

        std::optional<SpiderTeleportData> spiderTp1, spiderTp2;
        bool didJustJumpp1 = false, didJustJumpp2 = false;
        bool isCurrentlyDead = false;
        bool isLastDeathReal = false;
        bool firstReceivedData = true;
        float lastDeathTimestamp = 0.f;
        uint8_t deathCounter = 0;

        // api stuff
        std::vector<globed::callbacks::PlayerJoinFn> playerJoinCallbacks;
        std::vector<globed::callbacks::PlayerJoinFn> playerLeaveCallbacks;

        // ui elements
        GlobedOverlay* overlay = nullptr;
        std::unordered_map<int, RemotePlayer*> players;
        Ref<PlayerProgressIcon> selfProgressIcon = nullptr;
        Ref<CCNode> progressBarWrapper = nullptr;
        Ref<PlayerStatusIcons> selfStatusIcons = nullptr;
        Ref<GlobedVoiceOverlay> voiceOverlay = nullptr;
        //Ref<GlobedChatOverlay> chatOverlay = nullptr;
        Ref<GlobedNameLabel> ownNameLabel = nullptr;
        Ref<GlobedNameLabel> ownNameLabel2 = nullptr;
        Ref<cocos2d::CCSprite> noticeAlert = nullptr;
        bool showingNoticeAlert = false;

        // speedhack detection
        float lastKnownTimeScale = 1.0f;
        std::unordered_map<int, asp::time::Instant> lastSentPacket;

        // chat messages (duh)
        std::vector<std::pair<int, std::string>> chatMessages;

        std::vector<GlobedCounterChange> pendingCounterChanges;

        // readonly custom items
        int lastJoinedPlayer = 0; // 1
        int lastLeftPlayer = 0;   // 2
        int totalJoins = 0;
        int totalLeaves = 0;
    };

    static void onModify(auto& self) {
        GLOBED_MANAGE_HOOK(Gameplay, GJBaseGameLayer::checkCollisions);
        GLOBED_MANAGE_HOOK(Gameplay, GJBaseGameLayer::updateCamera);
    }

    $override
    bool init();

    $override
    int checkCollisions(PlayerObject* player, float dt, bool p2);

    // $override
    // void loadLevelSettings();

    $override
    void updateCamera(float dt);

    // vmt hook
    void onEnterHook();

    // Convenience getter for static cast to GlobedGJBGL
    static GlobedGJBGL* get();

    /* setup stuff to make init() cleaner */
    // all are ran in this order.

    void setupPreInit(GJGameLevel* level, bool editor);

    void setupAll();

    void setupBare();
    void setupDeferredAssetPreloading();
    void setupAudio();
    void setupMisc();
    void setupUpdate();
    void setupUi();
    void setupPacketListeners();

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

    static cocos2d::CCPoint getCameraDirectionVector();
    static float getCameraDirectionAngle();

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

    void notifyDeath();
    void setNoticeAlertActive(bool active);

    // for global triggers
    int countForCustomItem(int id);
    void updateCountersForCustomItem(int id);
    void updateCustomItem(int id, int value);

    // runs every frame while paused
    void pausedUpdate(float dt);

    template <typename T> requires (std::is_base_of_v<BaseGameplayModule, T>)
    void addModule() {
        m_fields->modules.push_back(std::make_unique<T>(this));
    }

    // With speedhack enabled, all scheduled selectors will run more often than they are supposed to.
    // This means, if you turn up speedhack to let's say 100x, you will send 3000 packets per second. That is a big no-no.
    // For naive speedhack implementations, we simply check CCScheduler::getTimeScale and properly reschedule our data sender.
    //
    // For non-naive speedhacks however, ones that don't use CCScheduler::setTimeScale, it is more complicated.
    // We record the time of sending each packet and compare the intervals. If the interval is suspiciously small, we reject the packet.
    // This does result in less smooth experience with non-naive speedhacks however.
    bool accountForSpeedhack(int uniqueKey, float cap, float allowance = 0.9f);

    void unscheduleSelectors();
    void unscheduleSelector(cocos2d::SEL_SCHEDULE selector);

    void rescheduleSelectors();
    void customSchedule(cocos2d::SEL_SCHEDULE, float interval = 0.f);

    void queueCounterChange(const GlobedCounterChange& change);

    Fields& getFields();

    void explodeRandomPlayer();
    void addPlayerJoinCallback(globed::callbacks::PlayerJoinFn fn);
    void addPlayerLeaveCallback(globed::callbacks::PlayerLeaveFn fn);

    void setPlayerVisibility(bool enabled);
};
