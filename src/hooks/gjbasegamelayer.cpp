#include "gjbasegamelayer.hpp"

#include <Geode/loader/Dispatch.hpp>

#include "game_manager.hpp"
#include "gjgamelevel.hpp"
#include <audio/all.hpp>
#include <managers/block_list.hpp>
#include <managers/error_queues.hpp>
#include <managers/friend_list.hpp>
#include <managers/profile_cache.hpp>
#include <managers/game_server.hpp>
#include <managers/settings.hpp>
#include <managers/popup.hpp>
#include <managers/room.hpp>
#include <data/packets/client/game.hpp>
#include <data/packets/client/general.hpp>
#include <data/packets/server/game.hpp>
#include <game/module/all.hpp>
#include <game/camera_state.hpp>
#include <hooks/game_manager.hpp>
#include <hooks/triggers/gjeffectmanager.hpp>
#include <ui/menu/settings/settings_layer.hpp>
#include <ui/menu/settings/connection_test_popup.hpp>
#include <util/math.hpp>
#include <util/debug.hpp>
#include <util/cocos.hpp>
#include <util/format.hpp>
#include <util/lowlevel.hpp>
#include <util/rng.hpp>

using namespace geode::prelude;
using namespace asp::time;

#define GLOBED_INVALID_THIS [&]{ static_assert(false, "`this` cannot be safely used inside those selectors, they are scheduled on the scene"); return decltype(this){}; }()

// how many units before the voice disappears
constexpr float PROXIMITY_VOICE_LIMIT = 1200.f;

constexpr float VOICE_OVERLAY_PAD_X = 5.f;
constexpr float VOICE_OVERLAY_PAD_Y = 20.f;

// TODO: dont do custom item shit if it's not enabled in the level (scan thru all objects n stuff)

// post an event to all modules
#define GLOBED_EVENT(self, code) \
    for (auto& module : self->m_fields->modules) { \
        module->code; \
    }

bool GlobedGJBGL::init() {
    if (!GJBaseGameLayer::init()) return false;

    // yeah so like

    auto* gm = static_cast<HookedGameManager*>(globed::cachedSingleton<GameManager>());
    gm->setLastSceneEnum();

    return true;
}

void GlobedGJBGL::onEnterHook() {
    // when unpausing regularly, this is true, otherwise false
    auto weRunningScene = this->getParent() == CCScene::get();

    if (weRunningScene) {
        CCLayer::onEnter();
        return;
    }

    Loader::get()->queueInMainThread([self = Ref(this)] {
        // TODO: i forgot why i don't use `self` here and also apparently GlobedGJBGL::get can be null here for 1 person in the world
        auto l = GlobedGJBGL::get();
        bool isPaused = l ? l->isPaused(false) : self->isPaused(false);

        if (!isPaused) {
            self->CCLayer::onEnter();
        }
    });
}

GlobedGJBGL* GlobedGJBGL::get() {
    return static_cast<GlobedGJBGL*>(globed::cachedSingleton<GameManager>()->m_gameLayer);
}

/* Setup */

// Runs before PlayLayer::init
void GlobedGJBGL::setupPreInit(GJGameLevel* level, bool editor) {
    auto& nm = NetworkManager::get();
    auto& settings = GlobedSettings::get();
    auto& fields = this->getFields();
    fields.isEditor = editor;

    auto levelId = HookedGJGameLevel::getLevelIDFrom(level);
    fields.globedReady = nm.established() && levelId > 0;

    if (!settings.globed.editorSupport && level->m_levelType == GJLevelType::Editor) {
        fields.globedReady = false;
    }

    // enable/disable certain hooks for performance

    if (fields.globedReady) {
        HookManager::get().enableGroup(HookManager::Group::Gameplay);
    } else {
        HookManager::get().disableGroup(HookManager::Group::Gameplay);
    }

#ifdef GLOBED_GP_CHANGES
    globed::toggleTriggerHooks(fields.globedReady);
#endif

    if (fields.globedReady) {
        // room settings
        fields.roomSettings = RoomManager::get().getInfo().settings;

        /* initialize modules */

        // discord rpc
        if (Loader::get()->isModLoaded("techstudent10.discord_rich_presence")
            && settings.globed.useDiscordRPC)
        {
            this->addModule<DiscordRpcModule>();
        }

        auto& rs = fields.roomSettings.flags;

        // Collision
        if (rs.collision) {
            bool shouldEnableCollision = true;

            constexpr bool forcedPlatformer = false; // TODO maybe reenable?
            if (!level->isPlatformer() && forcedPlatformer) {
                fields.forcedPlatformer = true;
#ifdef GEODE_IS_ANDROID
                if (this->m_uiLayer) {
                    this->m_uiLayer->togglePlatformerMode(true);
                }
#endif
            } else if (!level->isPlatformer()) {
                // not forced platformer, so disable
                shouldEnableCollision = false;
            }

            if (shouldEnableCollision) {
                this->addModule<CollisionModule>();
            }
        }

        // 2 player mode
        if (rs.twoPlayerMode && !editor) {
            this->addModule<TwoPlayerModeModule>();
        }

        // Deathlink
        if (rs.deathlink && !rs.twoPlayerMode && !editor) {
            this->addModule<DeathlinkModule>();
        }

        GLOBED_EVENT(this, setupPreInit(level));
    }
}

void GlobedGJBGL::setupAll() {
    auto& fields = this->getFields();

    this->setupBare();

    if (!fields.globedReady) return;

    this->setupDeferredAssetPreloading();

    this->setupAudio();

    this->setupMisc();

    this->setupUpdate();

    this->setupUi();
}

// runs even if not connected to a server or in a non-uploaded editor level
void GlobedGJBGL::setupBare() {
    auto& fields = this->getFields();

    Build<GlobedOverlay>::create()
        .scale(0.4f)
        .zOrder(11)
        .id("game-overlay"_spr)
        .parent(this)
        .store(fields.overlay);

    auto& nm = NetworkManager::get();

    // if not authenticated, do nothing
    auto levelId = HookedGJGameLevel::getLevelIDFrom(m_level);

    if (!nm.established()) {
        fields.overlay->updateWithDisconnected();
    } else if (!fields.globedReady) {
        fields.overlay->updateWithEditor();
    } else {
        // else update the overlay with ping
        fields.overlay->updatePing(GameServerManager::get().getActivePing());
    }

    GLOBED_EVENT(this, setupBare());
}

void GlobedGJBGL::setupDeferredAssetPreloading() {
    GlobedSettings& settings = GlobedSettings::get();

    auto* gm = static_cast<HookedGameManager*>(globed::cachedSingleton<GameManager>());

    if (util::cocos::shouldTryToPreload(false)) {
        log::info("Preloading assets (deferred)");

        auto start = Instant::now();

        util::cocos::preloadAssets(util::cocos::AssetPreloadStage::AllWithoutDeathEffects);

        log::info("Asset preloading took {}", start.elapsed().toString());

        gm->setAssetsPreloaded(true);
    }

    // load death effects if those were deferred too
    bool shouldLoadDeaths = settings.players.deathEffects && !settings.players.defaultDeathEffect;

    if (!util::cocos::forcedSkipPreload() && shouldLoadDeaths && !gm->getDeathEffectsPreloaded()) {
        log::info("Preloading death effects (deferred)");

        auto start = Instant::now();

        util::cocos::preloadAssets(util::cocos::AssetPreloadStage::DeathEffect);

        log::info("Death effect preloading took {}", start.elapsed().toString());
        gm->setDeathEffectsPreloaded(true);
    }

    util::cocos::cleanupThreadPool();
}

void GlobedGJBGL::setupAudio() {
#ifdef GLOBED_VOICE_SUPPORT

    GlobedSettings& settings = GlobedSettings::get();
    auto winSize = CCDirector::get()->getWinSize();

    if (settings.communication.voiceEnabled) {
        auto& vm = GlobedAudioManager::get();
# ifdef GLOBED_VOICE_CAN_TALK
        // set the audio device
        try {
            vm.setActiveRecordingDevice(settings.communication.audioDevice);
        } catch (const std::exception& e) {
            // try default device, if we have no mic then just do nothing
            try {
                vm.setActiveRecordingDevice(0);
            } catch (const std::exception& _e) {
                settings.communication.audioDevice = -1;
            }

            settings.communication.audioDevice = 0;
        }

        // set the record buffer size
        vm.setRecordBufferCapacity(settings.communication.lowerAudioLatency ? EncodedAudioFrame::LIMIT_LOW_LATENCY : EncodedAudioFrame::LIMIT_REGULAR);

        // start passive voice recording
        auto& vrm = VoiceRecordingManager::get();
        vrm.startRecording();
# endif // GLOBED_VOICE_CAN_TALK

        if (settings.levelUi.voiceOverlay) {
            m_fields->voiceOverlay = Build<GlobedVoiceOverlay>::create()
                .parent(m_uiLayer)
                .pos(winSize.width - VOICE_OVERLAY_PAD_X, VOICE_OVERLAY_PAD_Y)
                .anchorPoint(1.f, 0.f)
                .collect();
        }
    }

    GLOBED_EVENT(this, setupAudio());

#endif // GLOBED_VOICE_SUPPORT

        /*m_fields->chatOverlay = Build<GlobedChatOverlay>::create()
            .parent(m_uiLayer)
            .pos(winSize.width + 35.f, winSize.height / 2.f)
            .scale(0.7f)
            .anchorPoint(0.f, 0.f)
            .collect();*/
}

void GlobedGJBGL::setupPacketListeners() {
    auto& nm = NetworkManager::get();

    nm.addListener<PlayerProfilesPacket>(this, [](std::shared_ptr<PlayerProfilesPacket> packet) {
        auto& pcm = ProfileCacheManager::get();
        for (auto& player : packet->players) {
            pcm.insert(player);
        }
    });

    nm.addListener<LevelDataPacket>(this, [this](std::shared_ptr<LevelDataPacket> packet){
        auto& fields = this->getFields();

        fields.lastServerUpdate = fields.timeCounter;
        bool firstPacket = util::misc::swapFlag(fields.firstReceivedData);

        for (const auto& player : packet->players) {
            if (!fields.players.contains(player.accountId)) {
                // new player joined
                this->handlePlayerJoin(player.accountId);
            }

            fields.interpolator->updatePlayer(player.accountId, player.data, fields.lastServerUpdate);
        }

#ifdef GLOBED_GP_CHANGES
        if (packet->customItems) {
            for (const auto& [itemId, value] : *packet->customItems) {
                static_cast<GJEffectManagerHook*>(m_effectManager)->applyItem(
                    globed::customItemToItemId(itemId),
                    value
                );
            }
        }

        if (firstPacket) {
            fields.lastJoinedPlayer = GJAccountManager::get()->m_accountID;
            this->updateCountersForCustomItem(globed::ITEM_LAST_JOINED);
        }
#endif
    });

    nm.addListener<LevelPlayerMetadataPacket>(this, [this](std::shared_ptr<LevelPlayerMetadataPacket> packet) {
        for (const auto& player : packet->players) {
            this->m_fields->playerStore->insertOrUpdate(player.accountId, player.data.attempts, player.data.localBest);
        }
    });

    nm.addListener<LevelInnerPlayerCountPacket>(this, [this](std::shared_ptr<LevelInnerPlayerCountPacket> packet) {
        this->getFields().initialPlayerCount = packet->count;
    });

    nm.addListener<ChatMessageBroadcastPacket>(this, [this](std::shared_ptr<ChatMessageBroadcastPacket> packet) {
        this->m_fields->chatMessages.push_back({packet->sender, packet->message});

        //m_fields->chatOverlay->addMessage(packet->sender, packet->message);
    });

    nm.addListener<VoiceBroadcastPacket>(this, [this](std::shared_ptr<VoiceBroadcastPacket> packet) {
#ifdef GLOBED_VOICE_SUPPORT
        // if deafened or voice is disabled, do nothing
        auto& settings = GlobedSettings::get();

        if (this->m_fields->deafened || !settings.communication.voiceEnabled) return;
        if (!this->shouldLetMessageThrough(packet->sender)) return;

        auto& vpm = VoicePlaybackManager::get();
        try {
            vpm.prepareStream(packet->sender);

            vpm.setVolume(packet->sender, settings.communication.voiceVolume);
            this->updateProximityVolume(packet->sender);
            auto result = vpm.playFrameStreamed(packet->sender, packet->frame);

            if (result.isErr()) {
                ErrorQueues::get().debugWarn(std::string("Failed to play a voice frame: ") + result.unwrapErr());
            }
        } catch(const std::exception& e) {
            ErrorQueues::get().debugWarn(std::string("Failed to play a voice frame: ") + e.what());
        }
#endif // GLOBED_VOICE_SUPPORT
    });

    nm.addListener<VoiceFailedPacket>(this, [this](std::shared_ptr<VoiceFailedPacket> packet) {
#ifdef GLOBED_VOICE_SUPPORT
        auto& fields = this->getFields();

        fields.lastVoiceFailure = fields.timeCounter;
#endif // GLOBED_VOICE_SUPPORT
    });


    GLOBED_EVENT(this, setupPacketListeners());
}

void GlobedGJBGL::setupUpdate() {
    auto& nm = NetworkManager::get();

    // update
    Loader::get()->queueInMainThread([&nm] {
        auto self = GlobedGJBGL::get();
        if (!self || !self->established()) return;

        // here we run the stuff that must run on a valid playlayer
        self->setupPacketListeners();

        // send LevelJoinPacket and RequestPlayerProfilesPacket

        auto levelId = HookedGJGameLevel::getLevelIDFrom(self->m_level);

        nm.send(LevelJoinPacket::create(levelId, self->m_level->m_unlisted, std::nullopt));

        // sometimes this can fail, no idea why lol, defer scheduling until postInitActions
        bool canSchedule = self->rescheduleSelectors();

        if (canSchedule) {
            self->getParent()->schedule(schedule_selector(GlobedGJBGL::selUpdate), 0.f);
        }

        self->scheduleOnce(schedule_selector(GlobedGJBGL::postInitActions), 0.25f);
    });
}

void GlobedGJBGL::setupMisc() {
    auto& settings = GlobedSettings::get();
    auto& nm = NetworkManager::get();
    auto& fields = this->getFields();

    fields.isVoiceProximity = m_level->isPlatformer() ? settings.communication.voiceProximity : settings.communication.classicProximity;

    // set the configured tps
    fields.configuredTps = nm.getServerTps();

    // interpolator
    fields.interpolator = std::make_unique<PlayerInterpolator>(InterpolatorSettings {
        .realtime = false,
        .isPlatformer = m_level->isPlatformer(),
        .expectedDelta = (1.0f / fields.configuredTps)
    });

    // player store
    fields.playerStore = std::make_unique<PlayerStore>();

    // send SyncIconsPacket if our icons have changed since the last time we sent it
    auto& pcm = ProfileCacheManager::get();
    pcm.setOwnDataAuto();
    if (pcm.pendingChanges) {
        pcm.pendingChanges = false;
        nm.send(SyncIconsPacket::create(pcm.getOwnData()));
    }

    // friendlist stuff
    auto& flm = FriendListManager::get();
    flm.maybeLoad();

    // refresh keybinds
    KeybindsManager::get().refreshBinds();

    // vmt hook
#ifdef GEODE_IS_WINDOWS
    static auto patch = [&]() -> Patch* {
        auto vtableidx = 85;
        auto p = util::lowlevel::vmtHook(&GlobedGJBGL::onEnterHook, this, vtableidx);
        if (p.isErr()) {
            log::warn("GJBGL vmt hook failed: {}", p.unwrapErr());
            return nullptr;
        } else {
            return p.unwrap();
        }
    }();

    if (patch && !patch->isEnabled()) {
        (void) patch->enable();
    }
#endif

    GLOBED_EVENT(this, setupMisc());

    // check if any modules disable progress
    bool shouldSafeMode = false;
    for (auto& module : fields.modules) {
        if (!module->shouldSaveProgress()) {
            shouldSafeMode = true;
            break;
        }
    }

    if (shouldSafeMode) {
        fields.progressForciblyDisabled = true;
        this->toggleSafeMode(true);
    }
}

void GlobedGJBGL::setupUi() {
    auto& settings = GlobedSettings::get();
    auto& pcm = ProfileCacheManager::get();
    auto& fields = this->getFields();

    // progress icon
    Build<CCNode>::create()
        .id("progress-bar-wrapper"_spr)
        .visible(settings.levelUi.progressIndicators)
        .zOrder(-1)
        .store(fields.progressBarWrapper);

    Build<PlayerProgressIcon>::create()
        .parent(fields.progressBarWrapper)
        .id("self-player-progress"_spr)
        .store(fields.selfProgressIcon);

    fields.selfProgressIcon->updateIcons(pcm.getOwnData());
    fields.selfProgressIcon->setForceOnTop(true);

    // status icons
    if (settings.players.statusIcons) {
        Build<PlayerStatusIcons>::create(255)
            .scale(0.8f)
            .anchorPoint(0.5f, 0.f)
            .pos(0.f, 25.f)
            .parent(m_objectLayer)
            .id("self-status-icon"_spr)
            .store(fields.selfStatusIcons);
    }

    // own name
    if (settings.players.showNames && settings.players.ownName) {
        auto ownData = pcm.getOwnAccountData();
        auto ownSpecial = pcm.getOwnSpecialData();

        Build<GlobedNameLabel>::create(ownData.name, ownSpecial)
            .parent(m_objectLayer)
            .id("self-name"_spr)
            .store(fields.ownNameLabel);

        fields.ownNameLabel->updateOpacity(settings.players.nameOpacity);

        if (settings.players.dualName) {
            Build<GlobedNameLabel>::create(ownData.name, ownSpecial)
                .visible(false)
                .parent(m_objectLayer)
                .id("self-name-p2"_spr)
                .store(fields.ownNameLabel2);

            fields.ownNameLabel2->updateOpacity(settings.players.nameOpacity);
        }
    }

    GLOBED_EVENT(this, setupUi());
}

void GlobedGJBGL::postInitActions(float) {
    auto& fields = this->getFields();
    if (!fields.didSchedule) {
        bool can = this->rescheduleSelectors();
        if (can) {
            this->getParent()->schedule(schedule_selector(GlobedGJBGL::selUpdate), 0.f);
            fields.didSchedule = true;
        } else {
            ErrorQueues::get().error("Failed to schedule some selectors. This should not happen. Globed will be disabled while in this level.");
            this->disableAndRevertModifications();
        }
    }

    if (fields.globedReady) {
        auto& nm = NetworkManager::get();
        nm.send(RequestPlayerProfilesPacket::create(0));

        fields.shouldRequestMeta = true;
    }

    GLOBED_EVENT(this, postInitActions());
}

/* Selectors */

// selSendPlayerData - runs tps (default 30) times per second
void GlobedGJBGL::selSendPlayerData(float) {
#define this GLOBED_INVALID_THIS

    auto self = GlobedGJBGL::get();

    if (!self || !self->established()) return;
    auto& fields = self->getFields();

    // if (!self->isCurrentPlayLayer()) return;
    // TODO: idk remove this?
    if (!self->accountForSpeedhack(0, 1.0f / fields.configuredTps, 0.8f)) return;

    fields.totalSentPackets++;
    // additionally, if there are no players on the level, we drop down to 1 time per second as an optimization
    // or if we are quitting the level

    if ((fields.players.empty() && fields.totalSentPackets % 30 != 15 && fields.pendingCounterChanges.empty()) || fields.quitting) return;

    auto data = self->gatherPlayerData();
    std::optional<PlayerMetadata> meta;
    if (util::misc::swapFlag(fields.shouldRequestMeta)) {
        meta = self->gatherPlayerMetadata();
    }

    NetworkManager::get().send(PlayerDataPacket::create(data, meta, std::move(fields.pendingCounterChanges)));
#undef this
}

// selSendPlayerMetadata - runs every 10 seconds
void GlobedGJBGL::selSendPlayerMetadata(float) {
#define this GLOBED_INVALID_THIS
    auto self = GlobedGJBGL::get();

    if (!self || !self->established()) return;
    auto& fields = self->getFields();

    // if there are no players or we are quitting from the level, don't send the packet
    if (fields.players.empty() || fields.quitting) return;

    m_fields->shouldRequestMeta = true;

#undef this
}

// selPeriodicalUpdate - runs 4 times a second, does various stuff
void GlobedGJBGL::selPeriodicalUpdate(float dt) {
#define this GLOBED_INVALID_THIS

    auto self = GlobedGJBGL::get();

    if (!self || !self->established()) return;
    auto& fields = self->getFields();
    // if (!self->isCurrentPlayLayer()) return;

    // update the overlay
    // we dont use gsm.getActivePing because that can throw if we hit some
    // crazy race condition and are disconnected but the established() call above returned true
    // (yes, this has happened)
    auto& gsm = GameServerManager::get();
    if (auto server = gsm.getActiveServer()) {
        fields.overlay->updatePing(server->ping);
    }

    auto& pcm = ProfileCacheManager::get();

    util::collections::SmallVector<int, 32> toRemove;

    auto sinceUpdate = fields.timeCounter - fields.lastServerUpdate;

    // if more than a second passed and there was only 1 player, they probably left
    if (fields.lastServerUpdate == 0.0f && sinceUpdate > 5.f && fields.initialPlayerCount >= 10 && !fields.shownFragmentationAlert) {
        fields.shownFragmentationAlert = true;

        // if there were any players on the level when we first joined, but we never got a packet with their data,
        // then we probably have an incorrect packet limit set and we need to fix this.

        auto limit = GlobedSettings::get().globed.fragmentationLimit.get();

        if (limit > 0 && limit < 60000) {
            // user set the limit manually, ignore this ig
            log::warn(
                "Missing players detected (should be {} but have none), not showing frag test because user has a custom limit set: {}",
                fields.initialPlayerCount, limit
            );
        } else {
            log::warn("Missing players detected (should be {} but have none), prompting for connection test", fields.initialPlayerCount);

            // force pause the game
            if (!self->isEditor() && !self->isPaused()) {
                static_cast<PlayLayer*>(static_cast<GJBaseGameLayer*>(self))->pauseGame(false);
            }

            // show alert
            PopupManager::get().quickPopup(
                "Globed Warning",
                "We have detected that there are <cr>issues</c> in the connection, and Globed is having troubles showing other players for you. Do you want to try and run a <cg>connection test</c>, which will likely solve this issue? This action will <cy>exit</c> the current level.",
                "No",
                "Yes",
                [](auto, bool res) {
                    if (!res) {
                        PopupManager::get().alert(
                            "Globed Warning",
                            "If you want to silence this warning in the future, open settings and set Fragmentation Limit to a different value, for example 1400."
                        ).showInstant();
                    } else {
                        util::ui::replaceScene(GlobedSettingsLayer::create(true));
                    }
                }
            ).showQueue();
        }
    } else if (sinceUpdate > 1.0f && fields.players.size() < 2) {
        for (const auto& [playerId, _] : fields.players) {
            toRemove.push_back(playerId);
        }

        for (int id : toRemove) {
            self->handlePlayerLeave(id);
        }
    } else {
        util::collections::SmallVector<int, 8> ids;

        // kick players that have left the level
        for (const auto& [playerId, remotePlayer] : fields.players) {
            // if the player doesnt exist in last LevelData packet, they have left the level
            if (fields.interpolator->isPlayerStale(playerId, fields.lastServerUpdate)) {
                toRemove.push_back(playerId);
                continue;
            }

            auto data = pcm.getData(playerId);

            if (!remotePlayer->isValidPlayer()) {
                if (data.has_value()) {
                    // if the profile data already exists in cache, use it
                    remotePlayer->updateAccountData(data.value(), true);
                    continue;
                }

                // request again if it has either been 5 seconds, or if the player just joined
                if (remotePlayer->getDefaultTicks() == 20) {
                    remotePlayer->setDefaultTicks(0);
                }

                if (remotePlayer->getDefaultTicks() == 0) {
                    if (ids.size() < ids.capacity()) {
                        ids.push_back(playerId);
                    }
                }

                remotePlayer->incDefaultTicks();
            } else if (data.has_value()) {
                // still try to see if the cache has changed
                remotePlayer->updateAccountData(data.value());
            }
        }

        if (ids.size() > 5) {
            NetworkManager::get().send(RequestPlayerProfilesPacket::create(0));
        } else {
            for (int id : ids) {
                NetworkManager::get().send(RequestPlayerProfilesPacket::create(id));
            }
        }

        for (int id : toRemove) {
            self->handlePlayerLeave(id);
        }
    }

    // update the ping to the server if overlay is enabled
    if (GlobedSettings::get().overlay.enabled) {
        NetworkManager::get().updateServerPing();
    }

    GLOBED_EVENT(self, selPeriodicalUpdate(dt));
#undef this
}

// selUpdate - runs every frame, increments the non-decreasing time counter, interpolates and updates players
void GlobedGJBGL::selUpdate(float timescaledDt) {
    // we cannot use `this` inside this function!!!!
#define this GLOBED_INVALID_THIS

    auto self = GlobedGJBGL::get();

    if (!self) return;

    // timescale silently changing dt isn't very good when doing network interpolation >_>
    // since timeCounter needs to agree with everyone else on how long a second is!
    float dt = timescaledDt / CCScheduler::get()->getTimeScale();

    auto& fields = self->getFields();

    PlayLayer* pl = nullptr;
    if (!fields.isEditor) {
        pl = static_cast<PlayLayer*>(static_cast<GJBaseGameLayer*>(self));
    }

    fields.camState.visibleOrigin = CCPoint{0.f, 0.f};
    fields.camState.visibleCoverage = CCDirector::get()->getWinSize();

    fields.camState.cameraOrigin = self->m_gameState.m_cameraPosition;
    fields.camState.zoom = self->m_objectLayer->getScale();

    // update ourselves
    auto accountId = globed::cachedSingleton<GJAccountManager>()->m_accountID;
    fields.playerStore->insertOrUpdate(
        accountId,
        self->m_level->m_attempts,
        static_cast<uint32_t>(self->m_level->isPlatformer() ? self->m_level->m_bestTime : self->m_level->m_normalPercent)
    );

    fields.timeCounter += dt;

    fields.interpolator->tick(dt);

    // update progress indicators only if in PlayLayer
    if (pl) {
        if (fields.progressBarWrapper->getParent() != nullptr) {
            fields.selfProgressIcon->updatePosition(pl->getCurrentPercent() / 100.f, self->m_isPracticeMode);
        } else if (pl->m_progressBar) {
            // progressbar may get initialized late
            pl->m_progressBar->addChild(fields.progressBarWrapper);
        }
    }

    auto& bl = BlockListManager::get();
    auto& vpm = VoicePlaybackManager::get();
    auto& settings = GlobedSettings::get();

    for (const auto [playerId, remotePlayer] : fields.players) {
        // this should never happen, yet somehow it does for that one person
        if (!fields.interpolator->hasPlayer(playerId)) {
            log::error("Interpolator is missing a player: {}", playerId);
            continue;
        }

        auto& vstate = fields.interpolator->getPlayerState(playerId);

        auto frameFlags = fields.interpolator->swapFrameFlags(playerId);

        bool isSpeaking = vpm.isSpeaking(playerId);
        remotePlayer->updateData(
            vstate,
            frameFlags,
            isSpeaking,
            isSpeaking ? vpm.getLoudness(playerId) : 0.f,
            fields.arePlayersHidden
        );

        // update progress icons
        if (pl) {
            // dont update if we are in a normal level and without a progressbar
            if (self->m_level->isPlatformer() || pl->m_progressBar->isVisible()) {
                remotePlayer->updateProgressIcon();
            }
        }

        // update voice proximity
        self->updateProximityVolume(playerId);

        GLOBED_EVENT(self, onUpdatePlayer(playerId, remotePlayer, frameFlags));
    }

    if (fields.selfStatusIcons) {
        float pos = (fields.ownNameLabel && fields.ownNameLabel->isVisible()) ? 40.f : 25.f;
        fields.selfStatusIcons->setPosition(self->m_player1->getPosition() + CCPoint{0.f, pos});
        bool recording = VoiceRecordingManager::get().isRecording();
        bool recordingFailing = std::fabs(fields.lastVoiceFailure - fields.timeCounter) < 5.f;

        fields.selfStatusIcons->updateStatus(false, false, recording, recordingFailing, false, 0.f);
    }

    if (fields.voiceOverlay) {
        fields.voiceOverlay->updateOverlaySoft();
    }

    // update self names
    if (fields.ownNameLabel) {
        auto dirVec = self->getCameraDirectionVector();
        auto dir = self->getCameraDirectionAngle();

        if (self->m_player1->m_isHidden || !self->m_player1->isVisible()) {
            fields.ownNameLabel->setVisible(false);
        } else {
            fields.ownNameLabel->setVisible(true);
            fields.ownNameLabel->setPosition(self->m_player1->getPosition() + dirVec * CCPoint{25.f, 25.f});
            fields.ownNameLabel->setRotation(dir);
        }

        if (fields.ownNameLabel2) {
            if (self->m_player2->m_isHidden || !self->m_gameState.m_isDualMode || !self->m_player2->isVisible()) {
                fields.ownNameLabel2->setVisible(false);
            } else {
                fields.ownNameLabel2->setVisible(true);
                fields.ownNameLabel2->setPosition(self->m_player2->getPosition() + dirVec * CCPoint{25.f, 25.f});
                fields.ownNameLabel->setRotation(dir);
            }
        }
    }

    // show a small alert icon if we have a pending notice
    bool hasNotices = ErrorQueues::get().hasPendingNotices();
    if (hasNotices != fields.showingNoticeAlert) {
        fields.showingNoticeAlert = hasNotices;
        self->setNoticeAlertActive(hasNotices);
    }

    GLOBED_EVENT(self, selUpdate(dt));

#undef this
}

// selUpdateEstimators - runs 30 times a second, updates audio stuff
void GlobedGJBGL::selUpdateEstimators(float dt) {
#define this GLOBED_INVALID_THIS

    auto* self = GlobedGJBGL::get();

    // update volume estimators
    VoicePlaybackManager::get().updateAllEstimators(dt);

    if (auto overlay = self->m_fields->voiceOverlay) {
        overlay->updateOverlay();
    }

    GLOBED_EVENT(self, selUpdateEstimators(dt));
#undef this
}

/* Player related functions */

SpecificIconData GlobedGJBGL::gatherSpecificIconData(PlayerObject* player) {
    auto& fields = this->getFields();

    PlayerIconType iconType = PlayerIconType::Cube;
    if (player->m_isShip) iconType = m_level->isPlatformer() ? PlayerIconType::Jetpack : PlayerIconType::Ship;
    else if (player->m_isBird) iconType = PlayerIconType::Ufo;
    else if (player->m_isBall) iconType = PlayerIconType::Ball;
    else if (player->m_isDart) iconType = PlayerIconType::Wave;
    else if (player->m_isRobot) iconType = PlayerIconType::Robot;
    else if (player->m_isSpider) iconType = PlayerIconType::Spider;
    else if (player->m_isSwing) iconType = PlayerIconType::Swing;

    auto* pobjInner = static_cast<CCNode*>(player->getChildren()->objectAtIndex(0));

    float rot = player->getRotation();

    bool isPlayer1 = player == m_player1;

    auto spiderTeleportData = isPlayer1 ? util::misc::swapOptional(fields.spiderTp1) : util::misc::swapOptional(fields.spiderTp2);
    bool didJustJump = isPlayer1 ? util::misc::swapFlag(fields.didJustJumpp1) : util::misc::swapFlag(fields.didJustJumpp2);

    bool isStationary = false;
    if (m_level->isPlatformer()) {
        isStationary = std::abs(player->m_platformerXVelocity) < 0.1;
    }

    return SpecificIconData {
        .position = player->getPosition(),
        .rotation = rot,

        .iconType = iconType,
        .isVisible = player->isVisible(),
        .isLookingLeft = player->m_isGoingLeft,
        .isUpsideDown = player->m_isSwing ? player->m_isUpsideDown : pobjInner->getScaleY() == -1.0f,
        .isDashing = player->m_isDashing,
        .isMini = player->m_vehicleSize != 1.0f,
        .isGrounded = player->m_isOnGround,
        .isStationary = isStationary,
        .isFalling = player->m_yVelocity < 0.0,
        .didJustJump = didJustJump,
        .isRotating = player->m_isRotating,
        .isSideways = player->m_isSideways,

        .spiderTeleportData = spiderTeleportData
    };
}

PlayerData GlobedGJBGL::gatherPlayerData() {
    auto& fields = this->getFields();

    bool isDead = m_player1->m_isDead || m_player2->m_isDead;
    fields.isCurrentlyDead = isDead;

    // this function (getCurrentPercent) only exists in playlayer and not the editor, so reimpl it
    auto getPercent = [&](){
        float percent;

        if (m_level->m_timestamp > 0) {
            percent = static_cast<float>(m_gameState.m_currentProgress) / m_level->m_timestamp * 100.f;
        } else {
            percent = m_player1->getPosition().x / m_levelLength * 100.f;
        }

        if (percent >= 100.f) {
            return 100.f;
        } else if (percent <= 0.f) {
            return 0.f;
        } else {
            return percent;
        }
    };

    float currentPercentage = getPercent() / 100.f;
    if (std::isnan(currentPercentage)) {
        currentPercentage = 0.f;
    }

    bool isInEditor = this->isEditor();
    bool isEditorBuilding = false;

    if (isInEditor) {
        isEditorBuilding = this->m_playbackMode == PlaybackMode::Not;
    }

    return PlayerData {
        .timestamp = fields.timeCounter,

        .player1 = this->gatherSpecificIconData(m_player1),
        .player2 = this->gatherSpecificIconData(m_player2),

        .deathCounter = (float) fields.deathCounter,

        .currentPercentage = currentPercentage,

        .isDead = isDead,
        .isPaused = this->isPaused(),
        .isPracticing = m_isPracticeMode,
        .isDualMode = m_gameState.m_isDualMode,
        .isInEditor = isInEditor,
        .isEditorBuilding = isEditorBuilding,
        .isLastDeathReal = fields.isLastDeathReal,
    };
}

PlayerMetadata GlobedGJBGL::gatherPlayerMetadata() {
    uint32_t localBest;
    if (m_level->isPlatformer()) {
        localBest = static_cast<uint32_t>(m_level->m_bestTime);
    } else {
        localBest = static_cast<uint32_t>(m_level->m_normalPercent);
    }

    return PlayerMetadata {
        .localBest = localBest,
        .attempts = m_level->m_attempts,
    };
}

CCPoint GlobedGJBGL::getCameraDirectionVector() {
    float dir = this->getCameraDirectionAngle();
    float rads = dir * M_PI / 180.f; // convert degrees to radians
    return CCPoint{std::sin(rads), std::cos(rads)};
}

float GlobedGJBGL::getCameraDirectionAngle() {
    bool rotateNames = GlobedSettings::get().players.rotateNames;
    if (!rotateNames) {
        return 0.f;
    }

    return -m_gameState.m_cameraAngle;
}

bool GlobedGJBGL::shouldLetMessageThrough(int playerId) {
    auto& settings = GlobedSettings::get();
    auto& flm = FriendListManager::get();
    auto& bl = BlockListManager::get();

    if (bl.isExplicitlyBlocked(playerId)) return false;
    if (bl.isExplicitlyAllowed(playerId)) return true;

    if (settings.communication.onlyFriends && !flm.isFriend(playerId)) return false;

    return true;
}

void GlobedGJBGL::updateProximityVolume(int playerId) {
    auto& fields = this->getFields();

    if (fields.deafened || !fields.isVoiceProximity) return;

    auto& vpm = VoicePlaybackManager::get();

    if (!fields.interpolator->hasPlayer(playerId)) {
        // if we have no knowledge on the player, set volume to 0
        vpm.setVolume(playerId, 0.f);
        return;
    }

    auto& vstate = fields.interpolator->getPlayerState(playerId);

    float distance = cocos2d::ccpDistance(m_player1->getPosition(), vstate.player1.position);
    float volume = 1.f - std::clamp(distance, 0.01f, PROXIMITY_VOICE_LIMIT) / PROXIMITY_VOICE_LIMIT;
    if (vstate.isInEditor) {
        volume = 1.f;
    }

    auto& settings = GlobedSettings::get();

    if (this->shouldLetMessageThrough(playerId)) {
        vpm.setVolume(playerId, volume * settings.communication.voiceVolume);
    }
}

void GlobedGJBGL::notifyDeath() {
    auto& fields = this->getFields();

    fields.deathCounter++;
    fields.isLastDeathReal = !fields.isFakingDeath;
}

void GlobedGJBGL::setNoticeAlertActive(bool active) {
    auto& fields = this->getFields();

    // Add the alert if it does not exist
    if (!fields.noticeAlert) {
        if (!fields.isEditor) {
            auto pl = static_cast<PlayLayer*>(static_cast<GJBaseGameLayer*>(this));

            auto pbm = this->m_uiLayer->getChildByID("pause-button-menu");
            if (!pbm) {
                log::warn("pause-button-menu not found, not toggling notice alert");
                return;
            }

            Build<CCSprite>::createSpriteName("geode.loader/info-alert.png")
                .scale(0.45f)
                .opacity(255)
                .pos(8.f, 8.f)
                .id("notice-alert"_spr)
                .parent(pbm)
                .store(fields.noticeAlert);
        } else {
            // Editor, TODO
        }
    }

    fields.noticeAlert->setVisible(active);
}

void GlobedGJBGL::handlePlayerJoin(int playerId) {
    auto& settings = GlobedSettings::get();
    auto& fields = this->getFields();

    PlayerProgressIcon* progressIcon = nullptr;
    PlayerProgressArrow* progressArrow = nullptr;

    bool platformer = m_level->isPlatformer();

    if (!platformer && settings.levelUi.progressIndicators) {
        Build<PlayerProgressIcon>::create()
            .zOrder(2)
            .id(util::cocos::spr(fmt::format("remote-player-progress-{}", playerId)))
            .parent(fields.progressBarWrapper)
            .store(progressIcon);
    } else if (platformer && settings.levelUi.progressIndicators) {
        Build<PlayerProgressArrow>::create()
            .zOrder(2)
            .id(util::cocos::spr(fmt::format("remote-player-progress-{}", playerId)))
            .parent(this)
            .store(progressArrow);
    }

    // if we are in the editor, hide the progress indicators
    if (this->isEditor()) {
        if (progressArrow) {
            progressArrow->setVisible(false);
        }

        if (progressIcon) {
            progressIcon->setVisible(false);
        }
    }

    auto* rp = Build<RemotePlayer>::create(&fields.camState, progressIcon, progressArrow)
        .zOrder(10)
        .id(util::cocos::spr(fmt::format("remote-player-{}", playerId)))
        .collect();

    auto& pcm = ProfileCacheManager::get();
    auto pcmData = pcm.getData(playerId);
    if (pcmData.has_value()) {
        rp->updateAccountData(pcmData.value(), true);
    }

    auto& bl = BlockListManager::get();
    if (bl.isHidden(playerId)) {
        rp->setForciblyHidden(true);
    }

    m_objectLayer->addChild(rp);
    fields.players.emplace(playerId, rp);
    fields.interpolator->addPlayer(playerId);

    fields.lastJoinedPlayer = playerId;
    fields.totalJoins++;

#ifdef GLOBED_GP_CHANGES
    this->updateCustomItem(globed::ITEM_LAST_JOINED, fields.lastJoinedPlayer);
    this->updateCustomItem(globed::ITEM_TOTAL_PLAYERS_JOINED, fields.totalJoins);
    this->updateCustomItem(globed::ITEM_TOTAL_PLAYERS, fields.players.size() + 1);
#endif

    for (auto& cb : fields.playerJoinCallbacks) {
        cb(playerId, rp->player1->getPlayerObject(), rp->player2->getPlayerObject());
    }

    GLOBED_EVENT(this, onPlayerJoin(rp));
}

void GlobedGJBGL::handlePlayerLeave(int playerId) {
    VoicePlaybackManager::get().removeStream(playerId);

    auto& fields = this->getFields();

    if (!fields.players.contains(playerId)) return;

    auto rp = fields.players.at(playerId);

    for (auto& cb : fields.playerLeaveCallbacks) {
        cb(playerId, rp->player1->getPlayerObject(), rp->player2->getPlayerObject());
    }

    GLOBED_EVENT(this, onPlayerLeave(rp));

    rp->removeProgressIndicators();
    rp->cleanupObjectLayer();
    rp->removeFromParent();

    fields.players.erase(playerId);
    fields.interpolator->removePlayer(playerId);
    fields.playerStore->removePlayer(playerId);

    fields.lastLeftPlayer = playerId;
    fields.totalLeaves++;

#ifdef GLOBED_GP_CHANGES
    this->updateCustomItem(globed::ITEM_LAST_LEFT, fields.lastLeftPlayer);
    this->updateCustomItem(globed::ITEM_TOTAL_PLAYERS_LEFT, fields.totalLeaves);
    this->updateCustomItem(globed::ITEM_TOTAL_PLAYERS, fields.players.size() + 1);
#endif
}

bool GlobedGJBGL::established() {
    // the 2nd check is in case we disconnect while being in a level somehow
    return m_fields->globedReady && NetworkManager::get().established();
}

bool GlobedGJBGL::isCurrentPlayLayer() {
    auto playLayer = CCScene::get()->getChildByType<PlayLayer>(0);
    return static_cast<GJBaseGameLayer*>(playLayer) == this;
}

bool GlobedGJBGL::isPaused(bool checkCurrent) {
    if (this->isEditor()) {
        return this->m_playbackMode == PlaybackMode::Paused;
    }

    if (!m_fields->isEditor) {
        if (checkCurrent && !isCurrentPlayLayer()) return false;

        for (CCNode* child : CCArrayExt<CCNode*>(this->getParent()->getChildren())) {
            if (typeinfo_cast<PauseLayer*>(child)) {
                return true;
            }
        }
    }

    return false;
}

bool GlobedGJBGL::isEditor() {
    return (void*)globed::cachedSingleton<GameManager>()->m_levelEditorLayer == (void*)this;
}

bool GlobedGJBGL::isSafeMode() {
    return m_fields->shouldStopProgress;
}

void GlobedGJBGL::toggleSafeMode(bool enabled) {
    if (m_fields->progressForciblyDisabled) {
        enabled = true;
    }

    m_fields->shouldStopProgress = enabled;
}

void GlobedGJBGL::onQuitActions() {
    auto& nm = NetworkManager::get();

    m_fields->quitting = true;

    if (m_fields->globedReady) {
        if (nm.established()) {
            // send LevelLeavePacket
            nm.send(LevelLeavePacket::create());
        }

#ifdef GLOBED_VOICE_SUPPORT
        // stop voice recording and playback
        GlobedAudioManager::get().haltRecording();
        VoicePlaybackManager::get().stopAllStreams();
#endif // GLOBED_VOICE_SUPPORT

        GLOBED_EVENT(this, onQuit());
    }
}

void GlobedGJBGL::disableAndRevertModifications() {
    auto& fields = this->getFields();

    fields.globedReady = false;
    fields.modules.clear();
    fields.players.clear();
    HookManager::get().disableGroup(HookManager::Group::Gameplay);

    if (fields.overlay) {
        fields.overlay->updateWithDisconnected();
    }

    VoiceRecordingManager::get().stopRecording();
    VoicePlaybackManager::get().stopAllStreams();
    GlobedAudioManager::get().haltRecording();

    auto& nm = NetworkManager::get();
    if (nm.established()) {
        // send LevelLeavePacket
        nm.send(LevelLeavePacket::create());
    }

    this->unscheduleSelectors();
}

void GlobedGJBGL::pausedUpdate(float dt) {
    // unpause dash effects and death effects
    for (auto* child : CCArrayExt<CCNode*>(m_objectLayer->getChildren())) {
        int tag1 = ComplexVisualPlayer::SPIDER_DASH_CIRCLE_WAVE_TAG;
        int tag2 = ComplexVisualPlayer::SPIDER_DASH_SPRITE_TAG;
        int tag3 = ComplexVisualPlayer::DEATH_EFFECT_TAG;

        int ctag = child->getTag();

        if (ctag == tag1 || ctag == tag2 || ctag == tag3) {
            child->resumeSchedulerAndActions();
        }
    }
}

bool GlobedGJBGL::accountForSpeedhack(int uniqueKey, float cap, float allowance) {
    auto* sched = CCScheduler::get();
    auto& fields = this->getFields();
    auto ts = sched->getTimeScale();

    if (!util::math::equal(ts, fields.lastKnownTimeScale)) {
        this->unscheduleSelectors();
        this->rescheduleSelectors();
    }

    if (!fields.lastSentPacket.contains(uniqueKey)) {
        fields.lastSentPacket.emplace(uniqueKey, Instant::now());
        return true;
    }

    auto lastSent = fields.lastSentPacket.at(uniqueKey);

    auto passed = lastSent.elapsed().millis();

    if (passed < cap * 1000 * allowance) {
        // log::warn("dropping a packet (speedhack?), passed time is {}ms", passed);
        return false;
    }

    fields.lastSentPacket[uniqueKey] = Instant::now();

    return true;
}

void GlobedGJBGL::unscheduleSelectors() {
    auto* sched = CCScheduler::get();

    if (!this->getParent()) {
        return;
    }

    this->unscheduleSelector(schedule_selector(GlobedGJBGL::selSendPlayerData));
    this->unscheduleSelector(schedule_selector(GlobedGJBGL::selSendPlayerMetadata));
    this->unscheduleSelector(schedule_selector(GlobedGJBGL::selPeriodicalUpdate));
    this->unscheduleSelector(schedule_selector(GlobedGJBGL::selUpdateEstimators));

    GLOBED_EVENT(this, onUnscheduleSelectors());
}

void GlobedGJBGL::unscheduleSelector(cocos2d::SEL_SCHEDULE selector) {
    m_pScheduler->unscheduleSelector(selector, this->getParent());
}

bool GlobedGJBGL::rescheduleSelectors() {
    auto* sched = CCScheduler::get();
    float timescale = sched->getTimeScale();
    m_fields->lastKnownTimeScale = timescale;

    float pdInterval = (1.0f / m_fields->configuredTps) * timescale;
    float pmdInterval = 10.f * timescale;
    float updpInterval = 0.25f * timescale;
    float updeInterval = (1.0f / 30.f) * timescale;

    if (!this->getParent()) {
        m_fields->didSchedule = false;
        log::warn("Failed to schedule some selectors, will try again later");
        return false;
    }

    this->customSchedule(schedule_selector(GlobedGJBGL::selSendPlayerData), pdInterval);
    this->customSchedule(schedule_selector(GlobedGJBGL::selSendPlayerMetadata), pmdInterval);
    this->customSchedule(schedule_selector(GlobedGJBGL::selPeriodicalUpdate), updpInterval);
    this->customSchedule(schedule_selector(GlobedGJBGL::selUpdateEstimators), updeInterval);

    GLOBED_EVENT(this, onRescheduleSelectors(timescale));

    m_fields->didSchedule = true;

    return true;
}

void GlobedGJBGL::customSchedule(cocos2d::SEL_SCHEDULE selector, float interval) {
    if (auto parent = this->getParent()) {
        parent->schedule(selector, interval);
    }
}

void GlobedGJBGL::queueCounterChange(const GlobedCounterChange& change) {
    m_fields->pendingCounterChanges.push_back(change);
}

int GlobedGJBGL::countForCustomItem(int id) {
#ifdef GLOBED_GP_CHANGES
# define $id(x) (globed::ITEM_##x)
    auto& fields = this->getFields();

    switch (id) {
        case $id(ACCOUNT_ID): return GJAccountManager::get()->m_accountID;
        case $id(LAST_JOINED): return fields.lastJoinedPlayer;
        case $id(LAST_LEFT): return fields.lastLeftPlayer;
        case $id(TOTAL_PLAYERS): return fields.players.size();
        case $id(TOTAL_PLAYERS_JOINED): return fields.totalJoins;
        case $id(TOTAL_PLAYERS_LEFT): return fields.totalLeaves;
    }

    return 0;
# undef $id
#else
    return 0;
#endif
}

void GlobedGJBGL::updateCountersForCustomItem(int id) {
#ifdef GLOBED_GP_CHANGES
    static_cast<GJEffectManagerHook*>(m_effectManager)->updateCountersForCustomItem(id);
#endif
}

void GlobedGJBGL::updateCustomItem(int id, int value) {
#ifdef GLOBED_GP_CHANGES
    static_cast<GJEffectManagerHook*>(m_effectManager)->updateCountForItemCustom(id, value);
    this->updateCountersForCustomItem(id);
#endif
}

GlobedGJBGL::Fields& GlobedGJBGL::getFields() {
    return *m_fields.self();
}

// hooks

int GlobedGJBGL::checkCollisions(PlayerObject* player, float dt, bool p2) {
    int retval = GJBaseGameLayer::checkCollisions(player, dt, p2);

    if ((void*)this != GJBaseGameLayer::get()) return retval;
    if (!this->established()) return retval;

    GLOBED_EVENT(this, checkCollisions(player, dt, p2));

    return retval;
}

// TODO: this crashes when megahack force platformer is enabled
// go beg absolute to use geode patches.

// void GlobedGJBGL::loadLevelSettings() {
//     if (!GJBaseGameLayer::get()) {
//         GJBaseGameLayer::loadLevelSettings();
//         return;
//     }

//     GLOBED_EVENT(this, loadLevelSettingsPre());

//     GJBaseGameLayer::loadLevelSettings();

//     GLOBED_EVENT(this, loadLevelSettingsPost());
// }

class $modify(PlayerObject) {
    static void onModify(auto& self) {
        (void) self.setHookPriority("PlayerObject::playerDestroyed", -99999999).unwrap();

        GLOBED_MANAGE_HOOK(Gameplay, PlayerObject::update);
        GLOBED_MANAGE_HOOK(Gameplay, PlayerObject::playerDestroyed);
    }

    $override
    void update(float dt) {
        PlayerObject::update(dt);

        auto pl = GlobedGJBGL::get();
        if ((void*)m_gameLayer != pl || !pl) return;

        if (pl->m_player1 == this || pl->m_player2 == this) {
            GLOBED_EVENT(pl, mainPlayerUpdate(this, dt));
        } else {
            GLOBED_EVENT(pl, onlinePlayerUpdate(this, dt));
        }
    }

    $override
    void playerDestroyed(bool p0) {
        PlayerObject::playerDestroyed(p0);

        auto* gjbgl = GlobedGJBGL::get();
        if (gjbgl && (this == gjbgl->m_player1 || this == gjbgl->m_player2)) {
            GLOBED_EVENT(gjbgl, playerDestroyed(this, p0));
            gjbgl->notifyDeath();
        }
    }
};

void GlobedGJBGL::updateCamera(float dt) {
    GLOBED_EVENT(this, updateCameraPre(dt));
    GJBaseGameLayer::updateCamera(dt);
    GLOBED_EVENT(this, updateCameraPost(dt));
}

void GlobedGJBGL::explodeRandomPlayer() {
    auto& fields = this->getFields();

    if (fields.players.empty()) return;

    auto it = fields.players.begin();
    std::advance(it, util::rng::Random::get().generate<size_t>(0, fields.players.size() - 1));

    auto* player = it->second;
}

void GlobedGJBGL::addPlayerJoinCallback(globed::callbacks::PlayerJoinFn fn) {
    this->getFields().playerJoinCallbacks.push_back(std::move(fn));
}

void GlobedGJBGL::addPlayerLeaveCallback(globed::callbacks::PlayerLeaveFn fn) {
    this->getFields().playerLeaveCallbacks.push_back(std::move(fn));
}

void GlobedGJBGL::setPlayerVisibility(bool enabled) {
    auto& fields = this->getFields();
    fields.arePlayersHidden = enabled;
}
