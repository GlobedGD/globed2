#include <globed/core/game/RemotePlayer.hpp>
#include <globed/core/SettingsManager.hpp>
#include <globed/core/RoomManager.hpp>
#include <globed/audio/AudioManager.hpp>
#include <globed/util/gd.hpp>
#include <core/preload/PreloadManager.hpp>
#include <core/hooks/GJBaseGameLayer.hpp>
#include <core/game/Interpolator.hpp>
#include <core/game/SettingCache.hpp>
#include <core/CoreImpl.hpp>

#include <UIBuilder.hpp>
#include <cue/Util.hpp>

using namespace geode::prelude;

namespace globed {

static auto& g_settings = CachedSettings::get();

RemotePlayer::RemotePlayer(int playerId, GJBaseGameLayer* gameLayer, CCNode* parentNode) : m_state(), m_parentNode(parentNode) {
    m_state.accountId = playerId;
    m_localPlayer = playerId == 0;
    m_data = DEFAULT_PLAYER_DATA;

    auto start = asp::Instant::now();

    Build<VisualPlayer>::create(gameLayer, this, m_parentNode, false, playerId == 0)
        .id(fmt::format("{}-player1", playerId).c_str())
        .parent(m_parentNode)
        .visible(false)
        .store(m_player1);

    Build<VisualPlayer>::create(gameLayer, this, m_parentNode, true, playerId == 0)
        .id(fmt::format("{}-player2", playerId).c_str())
        .parent(m_parentNode)
        .visible(false)
        .store(m_player2);

    auto taken = start.elapsed();
    log::trace("Created player in {}", taken);
#ifdef GLOBED_DEBUG
#endif

    m_player1->m_remotePlayer = this;
    m_player2->m_remotePlayer = this;

    this->beginDataUpdate();

    bool plat = gameLayer->m_level->isPlatformer();

    // progress icon
    if (!gameLayer->m_isEditor) {
        if (plat && globed::setting<bool>("core.level.progress-indicators-plat")) {
            m_progArrow = Build<ProgressArrow>::create()
                .zOrder(2)
                .id(fmt::format("remote-player-progress-{}"_spr, playerId))
                .parent(gameLayer);

        } else if (!plat && globed::setting<bool>("core.level.progress-indicators")) {
            auto gjbgl = GlobedGJBGL::get(gameLayer);

            m_progIcon = Build<ProgressIcon>::create()
                .zOrder(2)
                .id(fmt::format("remote-player-progress-{}"_spr, playerId))
                .parent(gjbgl->m_fields->m_progressBarContainer);

            if (playerId == 0) {
                m_progIcon->updateIcons(globed::getPlayerIcons());
                m_progIcon->setForceOnTop(true);
            }
        }
    }
}

void RemotePlayer::beginDataUpdate() {
    m_player1->updateDisplayData();
    m_player2->updateDisplayData();

    // immediately update icons if everything has been preloaded or if they are defaults
    bool preloaded = PreloadManager::get().iconsLoaded();
    if (preloaded || !m_dataInitialized) {
        this->doUpdateIcons();
        return;
    }

    // otherwise, temporarily replace icons with defaults and start loading in background
    m_pendingIcons = m_data.icons;
    m_data.icons = DEFAULT_PLAYER_DATA.icons;

    PreloadOptions options{};
    options.blocking = false;
    options.completionCallback = [wself = this->weak_from_this()] {
        auto self = wself.lock();
        if (!self) return;

        log::trace("finished load async for {}", self->displayData().accountId);
        self->m_data.icons = *self->m_pendingIcons;
        self->m_pendingIcons.reset();
        self->doUpdateIcons();
    };

    log::trace("load async for {}", this->displayData().accountId);
    auto& pm = PreloadManager::get();
    pm.loadIcons(*m_pendingIcons, std::move(options));
}

void RemotePlayer::doUpdateIcons() {
    m_player1->updateIcons();
    m_player2->updateIcons();

    cue::Icons ci{
        .id = m_data.icons.cube,
        .color1 = m_data.icons.color1.asIdx(),
        .color2 = m_data.icons.color2.asIdx(),
        .glowColor = m_data.icons.glowColor.asIdx(),
    };

    if (m_progArrow) m_progArrow->updateIcons(ci);
    if (m_progIcon) m_progIcon->updateIcons(ci);
}

RemotePlayer::~RemotePlayer() {
    cue::resetNode(m_progArrow);
    cue::resetNode(m_progIcon);

    m_player1->cleanupObjectLayer();
    m_player2->cleanupObjectLayer();
    m_player1->removeFromParent();
    m_player2->removeFromParent();
}

void RemotePlayer::update(const PlayerState& state, const GameCameraState& camState, const OutFlags& flags, bool forceHide, bool noCulling) {
    forceHide = forceHide || m_forceHide;

    bool hideIcon = forceHide;
    bool hideEverything = forceHide;

    // if it's the local player, only show the actual icon if the setting is enabled
    if (this->isLocal() && !forceHide) {
        hideIcon = !CachedSettings::get().ghostFollower;
    }

    m_state = state;

    m_player1Culled = !m_state.player1;
    m_player2Culled = !m_state.player2;

    if (m_state.player1) {
        m_player1->updateFromData(*m_state.player1, m_state, camState, hideIcon, hideEverything, noCulling);
    } else {
        m_player1->setVisible(false);
    }

    if (m_state.player2) {
        m_player2->updateFromData(*m_state.player2, m_state, camState, hideIcon, hideEverything, noCulling);
    } else {
        m_player2->setVisible(false);
    }

    // update progress icons

    auto shownOrHide = [&](auto node, bool extraCond, auto&& onShown) {
        if (!node) return;
        if (hideEverything || !extraCond) {
            node->setVisible(false);
        } else {
            node->setVisible(true);
            onShown();
        }
    };

    shownOrHide(m_progIcon, true, [&] {
        m_progIcon->updatePosition(state.progress(), state.isPracticing);
    });

    shownOrHide(m_progArrow, !m_localPlayer, [&] {
        m_progArrow->updatePosition(camState, m_player1->getLastPosition(), state.progress());
    });

    // if the player just died, handle death
    if (flags.death) {
        if (!hideIcon) {
            this->handleDeath(*flags.death);
        }
        CoreImpl::get().onPlayerDeath(GlobedGJBGL::get(), this, *flags.death);
        m_wasDead = true;
    }

    // if the player just respawned, call respawn handlers
    if (m_wasDead && !state.isDead) {
        CoreImpl::get().onPlayerRespawn(GlobedGJBGL::get(), this);
        m_wasDead = false;
    }

    // if the player teleported, play a spider dash animation
    if (!hideIcon) {
        if (flags.spiderP1) {
            m_player1->handleSpiderTp(*flags.spiderP1);
        }
        if (flags.spiderP2) {
            m_player2->handleSpiderTp(*flags.spiderP2);
        }

        // if the player jumped, play a jump animation
        if (flags.jumpP1) {
            m_player1->playPlatformerJump();
        }
        if (flags.jumpP2) {
            m_player2->playPlatformerJump();
        }
    }
}

void RemotePlayer::handleDeath(const PlayerDeath& death) {
    if (globed::setting<bool>("core.player.death-effects")) {
        m_player1->playDeathEffect();
    }
}

void RemotePlayer::handleSpiderTp(const SpiderTeleportData& tp, bool p1) {
    p1 ? m_player1->handleSpiderTp(tp) : m_player2->handleSpiderTp(tp);
}

bool RemotePlayer::isDataInitialized() const {
    return m_dataInitialized;
}

bool RemotePlayer::isDataOutdated() const {
    return m_dataOutdated;
}

void RemotePlayer::markDataOutdated() {
    m_dataOutdated = true;
}

bool RemotePlayer::isTeamInitialized() const {
    return m_teamId.has_value();
}

void RemotePlayer::initData(const PlayerDisplayData& data, bool outdated, uint16_t teamId) {
    m_dataInitialized = true;
    m_dataOutdated = outdated;
    m_data = data;

    if (g_settings.defaultDeathEffects) {
        m_data.icons.deathEffect = DEFAULT_DEATH;
    }

    this->beginDataUpdate();
}

void RemotePlayer::updateTeam(uint16_t teamId) {
    m_teamId = teamId;
    m_player1->updateTeam(teamId);
    m_player2->updateTeam(teamId);
}

bool RemotePlayer::isTeammate(bool whatWhenNoTeams) {
    auto& rm = RoomManager::get();

    if (!rm.getSettings().teams) {
        return whatWhenNoTeams;
    }

    return m_teamId.has_value() && *m_teamId == rm.getCurrentTeamId();
}

void RemotePlayer::setForceHide(bool hide) {
    m_forceHide = hide;
}

VisualPlayer* RemotePlayer::player1() {
    return m_player1;
}

VisualPlayer* RemotePlayer::player2() {
    return m_player2;
}

PlayerDisplayData& RemotePlayer::displayData() {
    return m_data;
}

int RemotePlayer::id() const {
    return m_data.accountId ?: m_state.accountId;
}

bool RemotePlayer::isLocal() const {
    return m_localPlayer;
}

bool RemotePlayer::isPlayer1Culled() {
    return m_player1Culled;
}

bool RemotePlayer::isPlayer2Culled() {
    return m_player2Culled;
}

void RemotePlayer::stopVoiceStream() {
    if (m_voiceStream) {
        m_voiceStream->stop();
    }
}

VoiceStream* RemotePlayer::getVoiceStream() {
    return m_voiceStream.get();
}

void RemotePlayer::playVoiceData(EncodedAudioFrame frame) {
    if (!m_voiceStream) {
        auto res = VoiceStream::create(weak_from_this());
        if (!res) {
            log::error("Failed to create voice stream for player {}: {}", m_state.accountId, res.unwrapErr());
            return;
        }

        m_voiceStream = *res;
        m_voiceStream->setKind(AudioKind::VoiceChat);

        // if the player is in editor, voice should be non-proximity
        if (m_player1->m_isEditor) {
            m_voiceStream->setGlobal(true);
        }

        m_voiceStreamMutex = std::make_shared<std::mutex>();
    }

    // audio decoding can be expensive, so offload it to a blocking task
    arc::spawnBlocking<void>([
        stream = m_voiceStream,
        mtx = m_voiceStreamMutex,
        frame = std::move(frame),
        id = m_state.accountId
    ] mutable {
        std::lock_guard lock(*mtx);
        auto res = stream->writeData(frame);
        if (!res) {
            log::warn("Failed to play voice data for player {}: {}", id, res.unwrapErr());
        }
    });

}

}
