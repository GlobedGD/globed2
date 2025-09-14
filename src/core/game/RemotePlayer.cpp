#include <globed/core/game/RemotePlayer.hpp>
#include <globed/core/SettingsManager.hpp>
#include <globed/core/RoomManager.hpp>
#include <core/hooks/GJBaseGameLayer.hpp>

#include <UIBuilder.hpp>
#include <cue/Util.hpp>

using namespace geode::prelude;

namespace globed {

static const PlayerDisplayData DUMMY_DATA = {
    .accountId = 0,
    .userId = 0,
    .username = "Player",
    .icons = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 3, NO_GLOW, 1, NO_TRAIL, NO_TRAIL }
};

RemotePlayer::RemotePlayer(int playerId, GJBaseGameLayer* gameLayer, CCNode* parentNode) : m_state(), m_parentNode(parentNode) {
    m_state.accountId = playerId;

    Build<VisualPlayer>::create(gameLayer, this, m_parentNode, false)
        .id(fmt::format("{}-player1", playerId).c_str())
        .parent(m_parentNode)
        .visible(false)
        .store(m_player1);

    Build<VisualPlayer>::create(gameLayer, this, m_parentNode, true)
        .id(fmt::format("{}-player2", playerId).c_str())
        .parent(m_parentNode)
        .visible(false)
        .store(m_player2);

    m_player1->m_remotePlayer = this;
    m_player2->m_remotePlayer = this;

    m_data = DUMMY_DATA;

    m_player1->updateDisplayData();
    m_player2->updateDisplayData();

    bool plat = gameLayer->m_level->isPlatformer();

    if (globed::setting<bool>("core.level.progress-indicators") && !gameLayer->m_isEditor) {
        if (plat) {
            m_progArrow = Build<ProgressArrow>::create()
                .zOrder(2)
                .id(fmt::format("remote-player-progress-{}"_spr, playerId))
                .parent(gameLayer);
        } else {
            auto gjbgl = GlobedGJBGL::get(gameLayer);

            m_progIcon = Build<ProgressIcon>::create()
                .zOrder(2)
                .id(fmt::format("remote-player-progress-{}"_spr, playerId))
                .parent(gjbgl->m_fields->m_progressBarContainer);
        }
    }
}

RemotePlayer::~RemotePlayer() {
    cue::resetNode(m_progArrow);
    cue::resetNode(m_progIcon);

    m_player1->cleanupObjectLayer();
    m_player2->cleanupObjectLayer();
    m_player1->removeFromParent();
    m_player2->removeFromParent();
}

void RemotePlayer::update(const PlayerState& state, const GameCameraState& camState, bool forceHide) {
    // TODO: impl forceHide

    m_state = state;

    if (m_state.player1) {
        m_player1->updateFromData(*m_state.player1, m_state, camState);
    }

    if (m_state.player2) {
        m_player2->updateFromData(*m_state.player2, m_state, camState);
    } else {
        m_player2->setVisible(false);
    }

    // update progress icons

    if (m_progIcon) {
        m_progIcon->updatePosition(state.progress(), state.isPracticing);
    }

    if (m_progArrow) {
        m_progArrow->updatePosition(camState, m_player1->getLastPosition(), state.progress());
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

bool RemotePlayer::isTeamInitialized() const {
    return m_teamId.has_value();
}

void RemotePlayer::initData(const PlayerDisplayData& data, uint16_t teamId) {
    m_dataInitialized = true;
    m_data = data;

    m_player1->updateDisplayData();
    m_player2->updateDisplayData();

    cue::Icons ci{
        .id = data.icons.cube,
        .color1 = data.icons.color1.asIdx(),
        .color2 = data.icons.color2.asIdx(),
        .glowColor = data.icons.glowColor.asIdx(),
    };

    if (m_progArrow) m_progArrow->updateIcons(ci);
    if (m_progIcon) m_progIcon->updateIcons(ci);
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

VisualPlayer* RemotePlayer::player1() {
    return m_player1;
}

VisualPlayer* RemotePlayer::player2() {
    return m_player2;
}

PlayerDisplayData& RemotePlayer::displayData() {
    return m_data;
}

}
