#include <globed/core/game/RemotePlayer.hpp>
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

    Build<VisualPlayer>::create(gameLayer, m_parentNode, false)
        .id(fmt::format("{}-player1", playerId).c_str())
        .parent(m_parentNode)
        .visible(false)
        .store(m_player1);

    Build<VisualPlayer>::create(gameLayer, m_parentNode, true)
        .id(fmt::format("{}-player2", playerId).c_str())
        .parent(m_parentNode)
        .visible(false)
        .store(m_player2);

    m_player1->m_remotePlayer = this;
    m_player2->m_remotePlayer = this;

    m_player1->updateDisplayData(DUMMY_DATA);
    m_player2->updateDisplayData(DUMMY_DATA);
}

RemotePlayer::~RemotePlayer() {
    m_player1->cleanupObjectLayer();
    m_player2->cleanupObjectLayer();
    m_player1->removeFromParent();
    m_player2->removeFromParent();
}

void RemotePlayer::update(const PlayerState& state, const GameCameraState& camState) {
    m_state = state;

    m_player1->updateFromData(m_state.player1, m_state, camState);

    if (m_state.player2) {
        m_player2->updateFromData(*m_state.player2, m_state, camState);
    } else {
        m_player2->setVisible(false);
    }
}

bool RemotePlayer::isDataInitialized() const {
    return m_dataInitialized;
}

void RemotePlayer::initData(const PlayerDisplayData& data) {
    m_dataInitialized = true;

    m_player1->updateDisplayData(data);
    m_player2->updateDisplayData(data);
}

}
