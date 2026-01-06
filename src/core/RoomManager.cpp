#include <globed/core/RoomManager.hpp>
#include <globed/core/data/Messages.hpp>
#include <globed/core/actions.hpp>
#include <core/net/NetworkManagerImpl.hpp>

#include <Geode/loader/Dispatch.hpp>

using namespace geode::prelude;

namespace globed {

void RoomManager::joinLevel(int levelId, int author, bool platformer, bool editorCollab) {
    auto& nm = NetworkManagerImpl::get();

    // check for warp context, join a specific server if warping
    auto wctx = globed::_getWarpContext();

    if (wctx.levelId() == levelId) {
        nm.sendJoinSession(wctx, author, platformer, editorCollab);
    } else if (auto srv = this->pickServerId()) {
        // construct a session ID
        auto id = SessionId::fromParts(*srv, m_roomId, levelId);
        nm.sendJoinSession(id, author, platformer, editorCollab);
    }
}

std::optional<SessionId> RoomManager::getEditorCollabId(GJGameLevel* level) {
    using LevelIdEvent = DispatchEvent<GJGameLevel*, int64_t*>;

    int64_t id = level->m_levelID;

    LevelIdEvent("dankmeme.globed2/setup-level-id", level, &id).post();

    if (id == level->m_levelID) {
        return std::nullopt;
    } else {
        return SessionId{(uint64_t)id};
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

SessionId RoomManager::makeSessionId(int levelId) {
    return SessionId::fromParts(this->pickServerId().value_or(0), m_roomId, levelId);
}

void RoomManager::reset() {
    this->resetValues();
}

bool RoomManager::isInGlobal() {
    return m_roomId == 0;
}

bool RoomManager::isInRoom() {
    return m_roomId != 0;
}

bool RoomManager::isInFollowerRoom() {
    return m_roomId != 0 && m_settings.isFollower;
}

bool RoomManager::isOwner() {
    return m_roomOwner == cachedSingleton<GJAccountManager>()->m_accountID;
}

uint32_t RoomManager::getRoomId() {
    return m_roomId;
}

uint16_t RoomManager::getCurrentTeamId() {
    return m_teamId;
}

std::optional<RoomTeam> RoomManager::getCurrentTeam() {
    return this->getTeam(m_teamId);
}

std::optional<RoomTeam> RoomManager::getTeam(uint16_t id) {
    if (m_settings.teams && id < m_teams.size()) {
        return m_teams[id];
    } else {
        return std::nullopt;
    }
}

std::optional<uint16_t> RoomManager::getTeamIdForPlayer(int player) {
    if (!m_settings.teams) {
        return std::nullopt;
    }

    if (player == cachedSingleton<GJAccountManager>()->m_accountID) {
        return m_teamId;
    }

    for (auto& [teamId, players] : m_teamMembers) {
        if (std::find(players.begin(), players.end(), player) != players.end()) {
            return teamId;
        }
    }

    return std::nullopt;
}

int RoomManager::getRoomOwner() {
    return m_roomOwner;
}

std::optional<uint8_t> RoomManager::pickServerId() {
    // depending on whether we are in a room or not, we need to either get the room's server ID or our preferred
    if (this->isInGlobal()) {
        if (auto serverId = NetworkManagerImpl::get().getPreferredServer()) {
            return *serverId;
        } else {
            log::warn("Failed to choose a server to join the level, no servers available");
            return std::nullopt;
        }
    } else {
        return m_settings.serverId;
    }
}

RoomSettings& RoomManager::getSettings() {
    return m_settings;
}

void RoomManager::setAttemptedPasscode(uint32_t code) {
    m_passcode = code;
}

uint32_t RoomManager::getPasscode() {
    return m_passcode;
}

SessionId RoomManager::getPinnedLevel() {
    return m_pinnedLevel;
}

RoomManager::RoomManager() {
    m_roomId = 0;
    m_roomName = "Global Room";

    auto& nm = NetworkManagerImpl::get();

    nm.listenGlobal<msg::RoomStateMessage>([this](const auto& msg) {
        if (msg.roomId != m_roomId) {
            this->resetValues();
            m_roomId = msg.roomId;

            // a change in rooms resets the warp context as well
            globed::_clearWarpContext();

            // if we are in a level, clear the current session
            if (GJBaseGameLayer::get()) {
                NetworkManagerImpl::get().sendLeaveSession();
            }
        }

        m_settings = msg.settings;
        m_teams = msg.teams;
        m_roomOwner = msg.roomOwner;
        m_pinnedLevel = SessionId{msg.pinnedLevel};
        m_roomName = msg.roomName;
        m_passcode = msg.passcode;

        if (!msg.players.empty()) {
            m_teamMembers.clear();

            for (const RoomPlayer& player : msg.players) {
                m_teamMembers[player.teamId].push_back(player.accountData.accountId);
            }
        }

        return ListenerResult::Continue;
    })->setPriority(-10000);

    nm.listenGlobal<msg::TeamChangedMessage>([this](const auto& msg) {
        m_teamId = msg.teamId;
        return ListenerResult::Continue;
    })->setPriority(-10000);

    nm.listenGlobal<msg::TeamChangedMessage>([this](const auto& msg) {
        m_teamId = msg.teamId;
        return ListenerResult::Continue;
    })->setPriority(-10000);

    nm.listenGlobal<msg::TeamMembersMessage>([this](const auto& msg) {
        m_teamMembers.clear();

        for (auto [id, teamId] : msg.members) {
            m_teamMembers[teamId].push_back(id);
        }

        return ListenerResult::Continue;
    })->setPriority(-10000);

    nm.listenGlobal<msg::TeamsUpdatedMessage>([this](const auto& msg) {
        m_teams = msg.teams;

        return ListenerResult::Continue;
    })->setPriority(-10000);

    nm.listenGlobal<msg::RoomSettingsUpdatedMessage>([this](const auto& msg) {
        m_settings = msg.settings;

        return ListenerResult::Continue;
    })->setPriority(-10000);

    nm.listenGlobal<msg::PinnedLevelUpdatedMessage>([this](const auto& msg) {
        m_pinnedLevel = SessionId{msg.id};

        return ListenerResult::Continue;
    })->setPriority(-10000);
}

void RoomManager::resetValues() {
    m_roomId = 0;
    m_roomName = "";
    m_roomOwner = 0;
    m_pinnedLevel = SessionId{};
    m_teamId = 0;
    m_passcode = 0;
    m_teamMembers.clear();
    m_teams.clear();
}

$on_mod(Loaded) { RoomManager::get(); }

}