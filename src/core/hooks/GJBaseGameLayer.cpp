#include "GJBaseGameLayer.hpp"
#include <globed/core/RoomManager.hpp>
#include <globed/core/PlayerCacheManager.hpp>
#include <globed/util/algo.hpp>
#include <core/CoreImpl.hpp>
#include <core/PreloadManager.hpp>
#include <core/net/NetworkManagerImpl.hpp>

#include <UIBuilder.hpp>
#include <asp/time/Instant.hpp>

using namespace geode::prelude;
using namespace asp::time;

namespace globed {

void GlobedGJBGL::setupPreInit(GJGameLevel* level, bool editor) {
    // TODO: editor stuff
    auto& fields = *m_fields.self();
    auto& nm = NetworkManagerImpl::get();
    fields.m_editor = editor;

    // determine if mulitplayer should be active
    fields.m_active = nm.isConnected() && level->m_levelID != 0;

    // TODO: disable multiplayer in editor depending on settings

    if (fields.m_active) {
        RoomManager::get().joinLevel(level->m_levelID);
        CoreImpl::get().onJoinLevel(this, level, editor);
    }
}

void GlobedGJBGL::setupPostInit() {
    auto& fields = *m_fields.self();
    this->setupNecessary();

    if (!fields.m_active) return;

    // setup everything else
    this->setupAssetLoading();
    this->setupAudio();
    this->setupUpdateLoop();
    this->setupUi();
    this->setupListeners();
}

void GlobedGJBGL::setupNecessary() {
    // TODO: ping overlay
}

void GlobedGJBGL::setupAssetLoading() {
    auto& pm = PreloadManager::get();
    pm.enterContext(PreloadContext::Level);

    if (pm.shouldPreload()) {
        log::info("Preloading assets (deferred)");

        auto start = Instant::now();
        pm.loadEverything();

        log::info("Asset preloading took {} ({} items loaded)", start.elapsed().toString(), pm.getLoadedCount());
    }

    pm.exitContext();
}

void GlobedGJBGL::setupAudio() {
    // TODO
}

void GlobedGJBGL::setupUpdateLoop() {
    auto& fields = *m_fields.self();
    fields.m_sendDataInterval = 1.f / 30.f; // TODO: make this configurable by the server

    // this has to be deferred. why? i don't know! but bugs happen otherwise
    Loader::get()->queueInMainThread([this] {
        auto self = GlobedGJBGL::get();
        if (!self || !self->active()) {
            return;
        }

        auto& fields = *self->m_fields.self();

        if (auto p = this->getParent()) {
            p->schedule(schedule_selector(GlobedGJBGL::selUpdateProxy), 0.f);
            fields.m_didSchedule = true;
        } else {
            log::warn("Failed to schedule update loop, parent is null, will try again later");
        }

        self->scheduleOnce(schedule_selector(GlobedGJBGL::selPostInitActions), 0.25f);
    });
}

void GlobedGJBGL::setupUi() {
    auto& fields = *m_fields.self();

    Build<CCNode>::create()
        .id("player-node"_spr)
        .parent(m_objectLayer)
        .store(fields.m_playerNode);
}

void GlobedGJBGL::setupListeners() {
    auto& fields = *m_fields.self();

    fields.m_levelDataListener = NetworkManagerImpl::get().listen<msg::LevelDataMessage>([this](const msg::LevelDataMessage& message) {
        this->onLevelDataReceived(message);
        return ListenerResult::Continue;
    });
}

void GlobedGJBGL::onQuit() {
    auto& fields = *m_fields.self();

    if (!fields.m_active) {
        return;
    }

    fields.m_active = false;
    CoreImpl::get().onLeaveLevel(this, fields.m_editor);
    RoomManager::get().leaveLevel();
}

void GlobedGJBGL::selUpdateProxy(float dt) {
    if (auto self = GlobedGJBGL::get()) {
        self->active() ? self->selUpdate(dt) : (void)0;
    }
}

void GlobedGJBGL::selUpdate(float tsdt) {
    float dt = tsdt / CCScheduler::get()->getTimeScale();
    auto& fields = *m_fields.self();

    fields.m_timeCounter += dt;

    float p1x = m_player1->getPosition().x;
    float p1xdiff = p1x - fields.m_lastP1XPosition;
    fields.m_lastP1XPosition = p1x;

    // process stuff
    fields.m_interpolator.tick(dt, p1xdiff);

    fields.m_unknownPlayers.clear();

    for (auto& it : fields.m_players) {
        int playerId = it.first;
        auto& player = it.second;

        if (!fields.m_interpolator.hasPlayer(playerId)) {
            log::error("Interpolator is missing a player: {}", playerId);
            continue;
        }

        auto& vstate = fields.m_interpolator.getPlayerState(playerId);
        player->update(vstate);

        // if the player has left the level, remove them
        if (fields.m_interpolator.isPlayerStale(playerId, fields.m_lastServerUpdate)) {
            this->handlePlayerLeave(playerId);
            continue;
        }

        // if we don't know player's data yet (username, icons, etc.), request it
        if (!player->isDataInitialized()) {
            fields.m_unknownPlayers.push_back(playerId);
        }
    }

    // the server might not send any updates if there are no players on the level,
    // if we receive no response for a while, assume all players have left
    if (fields.m_timeCounter - fields.m_lastServerUpdate > 1.25f && fields.m_players.size() <= 2) {
        for (auto& it : fields.m_players) {
            int playerId = it.first;
            this->handlePlayerLeave(playerId);
        }
    }

    // send player data to the server
    if (fields.m_timeCounter - fields.m_lastDataSend >= fields.m_sendDataInterval) {
        fields.m_lastDataSend += fields.m_sendDataInterval;
        this->selSendPlayerData(dt);
    }
}

void GlobedGJBGL::selPostInitActions(float dt) {
    auto& fields = *m_fields.self();

    if (!fields.m_didSchedule) {
        if (auto p = this->getParent()) {
            p->schedule(schedule_selector(GlobedGJBGL::selUpdateProxy), 0.f);
            fields.m_didSchedule = true;
        } else {
            log::error("Failed to schedule update loop, parent is null again");
        }
    }
}

void GlobedGJBGL::selSendPlayerData(float dt) {
    auto& fields = *m_fields.self();
    fields.m_totalSentPackets++;

    auto state = this->getPlayerState();
    std::vector<int> toRequest;
    float sinceRequest = fields.m_timeCounter - fields.m_lastDataRequest;

    // only request data if there's no in flight request or more than 1 second has passed since one was made (likely lost)
    if (fields.m_lastDataRequest == 0.f || sinceRequest > 1.f) {
        toRequest.reserve(std::min<size_t>(fields.m_unknownPlayers.size(), 64));

        for (int player : m_fields->m_unknownPlayers) {
            if (player <= 0 || toRequest.size() >= 64) {
                continue;
            }

            toRequest.push_back(player);
        }

        fields.m_lastDataRequest = fields.m_timeCounter;

        // TODO: technically there's a possibility for a "ghost player" where we think they are on the level, but the server is not aware of them,
        // this will cause them to be sent every single time (as the server will never send their data). not sure how to handle this yet.
    }

    NetworkManagerImpl::get().sendPlayerState(state, toRequest);
}

PlayerState GlobedGJBGL::getPlayerState() {
    auto& fields = *m_fields.self();

    PlayerState out{};
    out.accountId = globed::cachedSingleton<GJAccountManager>()->m_accountID;
    out.timestamp = fields.m_timeCounter;
    out.frameNumber = 0;
    out.deathCount = 0;

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

    double progress = (double)getPercent() / 100.0;
    if (std::isnan(progress) || std::isinf(progress)) {
        progress = 0.0;
    }

    out.percentage = static_cast<uint16_t>(std::floor(progress * 65535.0));
    out.isDead = m_player1->m_isDead || m_player2->m_isDead;
    out.isPaused = this->isPaused();
    out.isPracticing = m_isPracticeMode;
    out.isInEditor = this->isEditor();
    out.isEditorBuilding = out.isInEditor && m_playbackMode == PlaybackMode::Not;
    out.isLastDeathReal = true; // TODO: deathlink

    auto getPlayerObjState = [this, &fields](PlayerObject* obj, PlayerObjectData& out){
        using enum PlayerIconType;

        PlayerIconType iconType = Cube;
        if (obj->m_isShip) iconType = m_level->isPlatformer() ? Jetpack : Ship;
        else if (obj->m_isBall) iconType = Ball;
        else if (obj->m_isBird) iconType = Ufo;
        else if (obj->m_isDart) iconType = Wave;
        else if (obj->m_isRobot) iconType = Robot;
        else if (obj->m_isSpider) iconType = Spider;
        else if (obj->m_isSwing) iconType = Swing;
        out.iconType = iconType;

        auto pobjInner = static_cast<CCNode*>(obj->getChildren()->objectAtIndex(0));
        out.position = obj->getPosition();
        out.rotation = globed::normalizeAngle(obj->getRotation());

        out.isVisible = obj->isVisible();
        out.isLookingLeft = obj->m_isGoingLeft;
        out.isUpsideDown = obj->m_isSwing ? obj->m_isUpsideDown : pobjInner->getScaleY() == -1.0f;
        out.isDashing = obj->m_isDashing;
        out.isMini = obj->m_vehicleSize != 1.0f;
        out.isGrounded = obj->m_isOnGround;
        out.isStationary = m_level->isPlatformer() ? std::abs(obj->m_platformerXVelocity) < 0.1 : false;
        out.isFalling = obj->m_yVelocity < 0.0f;
        // TODO: set didJustJump
        out.isRotating = obj->m_isRotating;
        out.isSideways = obj->m_isSideways;
    };

    getPlayerObjState(m_player1, out.player1);

    if (m_gameState.m_isDualMode) {
        out.player2 = PlayerObjectData{};
        getPlayerObjState(m_player2, *out.player2);
    }

    return out;
}

bool GlobedGJBGL::isPaused(bool checkCurrent) {
    if (this->isEditor()) {
        return m_playbackMode == PlaybackMode::Paused;
    }

    if (checkCurrent && !this->isCurrentPlayLayer()) {
        return false;
    }

    for (CCNode* child : CCArrayExt<CCNode*>(this->getParent()->getChildren())) {
        if (typeinfo_cast<PauseLayer*>(child)) {
            return true;
        }
    }

    return false;
}

bool GlobedGJBGL::isEditor() {
    // return m_fields->m_editor;
    return m_isEditor;
}

bool GlobedGJBGL::isCurrentPlayLayer() {
    auto pl = CCScene::get()->getChildByType<PlayLayer>(0);
    return static_cast<GJBaseGameLayer*>(pl) == this;
}

void GlobedGJBGL::handlePlayerJoin(int playerId) {
    auto& fields = *m_fields.self();

    if (fields.m_players.contains(playerId)) {
        return;
    }

    auto rp = std::make_unique<RemotePlayer>(playerId, this, fields.m_playerNode);
    fields.m_players.emplace(playerId, std::move(rp));
    fields.m_interpolator.addPlayer(playerId);
}

void GlobedGJBGL::handlePlayerLeave(int playerId) {
    auto& fields = *m_fields.self();

    if (!fields.m_players.contains(playerId)) {
        return;
    }

    auto& player = fields.m_players.at(playerId);

    // TODO: cleanup
    fields.m_players.erase(playerId);
    fields.m_interpolator.removePlayer(playerId);
    PlayerCacheManager::get().evictToLayer2(playerId);
}

GlobedGJBGL* GlobedGJBGL::get(GJBaseGameLayer* base) {
    if (!base) {
        base = GJBaseGameLayer::get();
    }

    return static_cast<GlobedGJBGL*>(base);
}

bool GlobedGJBGL::active() {
    return m_fields->m_active;
}

void GlobedGJBGL::onLevelDataReceived(const msg::LevelDataMessage& message) {
    auto& fields = *m_fields.self();
    fields.m_lastServerUpdate = fields.m_timeCounter;

    for (auto& player : message.players) {
        if (player.accountId <= 0) continue;

        if (!fields.m_players.contains(player.accountId)) {
            this->handlePlayerJoin(player.accountId);
        }

        fields.m_interpolator.updatePlayer(player, fields.m_lastServerUpdate);
    }

    for (auto id : message.culled) {
        if (id <= 0) continue;

        // this player is still on the level, but the server decided not to send their data,
        // for example because they are stationary or very far away
        // call `updateNoop` to prevent them from being kicked
        fields.m_interpolator.updateNoop(id, fields.m_lastServerUpdate);
    }

    for (auto& dd : message.displayDatas) {
        if (dd.accountId <= 0) continue; // should never happen?

        PlayerCacheManager::get().insert(dd.accountId, dd);
    }

    if (!message.displayDatas.empty()) {
        fields.m_lastDataRequest = 0.f; // reset the time, so that we can request more players
    }
}

}
