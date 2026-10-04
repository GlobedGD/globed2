#include <globed/core/RoomManager.hpp>
#include <globed/core/data/Messages.hpp>
#include <globed/core/actions.hpp>
#include <globed/soft-link/Events.hpp>
#include <core/net/NetworkManagerImpl.hpp>

#include <Geode/loader/Dispatch.hpp>

using namespace geode::prelude;

namespace globed {

GLOBED_EXPORT_SINGLETON(RoomManager, SingletonLeakBase<RoomManager>);

std::optional<SessionId> RoomManager::getEditorCollabId(GJGameLevel* level) {
    int64_t id = level->m_levelID;

    SetupLevelIdEvent().send(level, &id);

    if (id == level->m_levelID) {
        return std::nullopt;
    } else {
        return SessionId{(uint64_t)id};
    }
}

SessionId RoomManager::makeSessionId(int levelId) {
    return SessionId::fromParts(this->pickServerId().value_or(0), this->getRoomId(), levelId);
}

std::optional<uint8_t> RoomManager::pickServerId() {
    // depending on whether we are in a room or not, we need to either get the room's server ID or our preferred
    if (this->isInGlobal()) {
        if (auto serverId = NetworkManagerImpl::get().getPreferredServer()) {
            return *serverId;
        } else {
            return std::nullopt;
        }
    } else {
        auto state = m_state.lock();
        return state->m_settings.serverId;
    }
}

void RoomManager::joinLevel(int levelId, int author, bool platformer, bool editorCollab) {
    auto& nm = NetworkManagerImpl::get();

    if (auto srv = this->pickServerId()) {
        // construct a session ID
        auto id = SessionId::fromParts(*srv, this->getRoomId(), levelId);
        nm.sendJoinSession(id, author, platformer, editorCollab);
    } else {
        log::warn("Failed to choose a server to join the level, no servers available");
    }
}

void RoomManager::joinLevel(GJGameLevel* level) {
    auto eid = getEditorCollabId(level);

    if (eid) {
        auto& nm = NetworkManagerImpl::get();
        nm.sendJoinSession(*eid, level->m_accountID, level->isPlatformer(), true);
    } else {
        this->joinLevel(level->m_levelID, level->m_accountID, level->isPlatformer(), false);
    }
}

void RoomManager::leaveLevel() {
    auto& nm = NetworkManagerImpl::get();
    nm.sendLeaveSession();
}

void RoomManager::reset() {
    m_state.lock()->resetValues();
}

bool RoomManager::isInGlobal() {
    return this->getRoomId() == 0;
}

bool RoomManager::isInRoom() {
    return this->getRoomId() != 0;
}

bool RoomManager::isInFollowerRoom() {
    auto state = m_state.lock();
    return state->m_roomId != 0 && state->m_settings.isFollower;
}

bool RoomManager::isOwner() {
    return this->getRoomOwner() == singleton<GJAccountManager>()->m_accountID;
}

uint32_t RoomManager::getRoomId() {
    return m_state.lock()->m_roomId;
}

uint16_t RoomManager::getCurrentTeamId() {
    return m_state.lock()->m_teamId;
}

std::optional<RoomTeam> RoomManager::getCurrentTeam() {
    return this->getTeam(this->getCurrentTeamId());
}

std::optional<RoomTeam> RoomManager::getTeam(uint16_t id) {
    auto state = m_state.lock();
    if (state->m_settings.teams && id < state->m_teams.size()) {
        return state->m_teams[id];
    } else {
        return std::nullopt;
    }
}

std::optional<uint16_t> RoomManager::getTeamIdForPlayer(int player) {
    auto state = m_state.lock();
    if (!state->m_settings.teams) {
        return std::nullopt;
    }

    if (player == singleton<GJAccountManager>()->m_accountID) {
        return state->m_teamId;
    }

    for (auto& [teamId, players] : state->m_teamMembers) {
        if (std::find(players.begin(), players.end(), player) != players.end()) {
            return teamId;
        }
    }

    return std::nullopt;
}

int32_t RoomManager::getRoomOwner() {
    return m_state.lock()->m_roomOwner;
}

std::string RoomManager::getRoomName() {
    return m_state.lock()->m_roomName;
}

RoomSettings RoomManager::getSettings() {
    return m_state.lock()->m_settings;
}

void RoomManager::setAttemptedPasscode(uint32_t code) {
    m_state.lock()->m_passcode = code;
}

uint32_t RoomManager::getPasscode() {
    return m_state.lock()->m_passcode;
}

SessionId RoomManager::getPinnedLevel() {
    return m_state.lock()->m_pinnedLevel;
}

SessionId RoomManager::getCurrentWarpLevel() {
    return m_state.lock()->m_currentWarpLevel;
}

RoomManager::RoomManager() {
    auto state = m_state.lock();
    state->m_roomId = 0;
    state->m_roomName = "Global Room";

    auto& nm = NetworkManagerImpl::get();

    nm.listenGlobal<msg::RoomStateMessage>([this](const auto& msg) {
        auto state = m_state.lock();
        if (msg.roomId != state->m_roomId) {
            state->resetValues();
            state->m_roomId = msg.roomId;

            // a change in rooms resets the preferred server
            NetworkManagerImpl::get().setTemporaryServerOverride(std::nullopt);

            // if we are in a level, clear the current session
            if (GJBaseGameLayer::get()) {
                NetworkManagerImpl::get().sendLeaveSession();
            }
        }

        state->m_settings = msg.settings;
        state->m_teams = msg.teams;
        state->m_roomOwner = msg.roomOwner;
        state->m_pinnedLevel = SessionId{msg.pinnedLevel};
        state->m_roomName = msg.roomName;
        state->m_passcode = msg.passcode;

        if (!msg.players.empty()) {
            state->m_teamMembers.clear();

            for (const RoomPlayer& player : msg.players) {
                state->m_teamMembers[player.teamId].push_back(player.accountData.accountId);

                // update the current warp level to the owner's level
                if (player.accountData.accountId == state->m_roomOwner) {
                    state->m_currentWarpLevel = player.session;
                }
            }
        }
    }, -10000);

    nm.listenGlobal<msg::TeamChangedMessage>([this](const auto& msg) {
        m_state.lock()->m_teamId = msg.teamId;
    }, -10000);

    nm.listenGlobal<msg::TeamMembersMessage>([this](const auto& msg) {
        auto state = m_state.lock();
        state->m_teamMembers.clear();

        for (auto [id, teamId] : msg.members) {
            state->m_teamMembers[teamId].push_back(id);
        }
    }, -10000);

    nm.listenGlobal<msg::TeamsUpdatedMessage>([this](const auto& msg) {
        m_state.lock()->m_teams = msg.teams;
    }, -10000);

    nm.listenGlobal<msg::RoomSettingsUpdatedMessage>([this](const auto& msg) {
        m_state.lock()->m_settings = msg.settings;
    }, -10000);

    nm.listenGlobal<msg::PinnedLevelUpdatedMessage>([this](const auto& msg) {
        m_state.lock()->m_pinnedLevel = SessionId{msg.id};
    }, -10000);

    nm.listenGlobal<msg::RoomWarpMessage>([this](const auto& msg) {
        m_state.lock()->m_currentWarpLevel = msg.sessionId;
        log::debug("Current warp level {}", msg.sessionId.levelId());
        globed::warpToSession(WarpContext{ msg.sessionId, WarpSource::Room });
    }, -10000);
}

void RoomManager::State::resetValues() {
    m_roomId = 0;
    m_roomName = "Global Room";
    m_roomOwner = 0;
    m_pinnedLevel = SessionId{};
    m_currentWarpLevel = SessionId{};
    m_teamId = 0;
    m_passcode = 0;
    m_teamMembers.clear();
    m_teams.clear();
}

$on_mod(Loaded) { RoomManager::get(); }

}