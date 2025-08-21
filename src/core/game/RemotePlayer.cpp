#include <globed/core/game/RemotePlayer.hpp>
#include <globed/core/SettingsManager.hpp>
#include <globed/core/RoomManager.hpp>
#include <UIBuilder.hpp>

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
}

RemotePlayer::~RemotePlayer() {
    m_player1->cleanupObjectLayer();
    m_player2->cleanupObjectLayer();
    m_player1->removeFromParent();
    m_player2->removeFromParent();
}

void RemotePlayer::update(const PlayerState& state, const GameCameraState& camState) {
    m_state = state;

    if (m_state.player1) {
        m_player1->updateFromData(*m_state.player1, m_state, camState);
    }

    if (m_state.player2) {
        m_player2->updateFromData(*m_state.player2, m_state, camState);
    } else {
        m_player2->setVisible(false);
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
    return m_player1->m_teamInitialized;
}

void RemotePlayer::initData(const PlayerDisplayData& data, uint16_t teamId) {
    m_dataInitialized = true;
    m_data = data;

    m_player1->updateDisplayData();
    m_player2->updateDisplayData();
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
