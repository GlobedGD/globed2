#include <globed/core/RoomManager.hpp>
#include <globed/core/SessionId.hpp>
#include <globed/core/data/Messages.hpp>
#include <globed/core/actions.hpp>
#include <core/net/NetworkManagerImpl.hpp>

using namespace geode::prelude;

namespace globed {

void RoomManager::joinLevel(int levelId) {
    auto& nm = NetworkManagerImpl::get();

    // check for warp context, join a specific server if warping
    auto wctx = globed::_getWarpContext();
    if (wctx.levelId() == levelId) {
        nm.sendJoinSession(wctx);
    } else if (auto srv = this->pickServerId()) {
        // construct a session ID
        auto id = SessionId::fromParts(*srv, m_roomId, levelId);
        nm.sendJoinSession(id);
    }
}

void RoomManager::leaveLevel() {
    auto& nm = NetworkManagerImpl::get();
    nm.sendLeaveSession();
}

bool RoomManager::isInGlobal() {
    return m_roomId == 0;
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

RoomManager::RoomManager() {
    m_roomId = 0;
    m_roomName = "Global Room";

    auto& nm = NetworkManagerImpl::get();

    nm.listenGlobal<msg::RoomStateMessage>([this](const auto& msg) {
        if (msg.roomId != m_roomId) {
            m_roomId = msg.roomId;
            m_roomName = msg.roomName;
            m_roomOwner = msg.roomOwner;
            m_teamId = 0;
            m_teamMembers.clear();

            // a change in rooms resets the warp context as well
            globed::_clearWarpContext();
        }

        m_settings = msg.settings;

        return ListenerResult::Continue;
    })->setPriority(-10000);

    nm.listenGlobal<msg::TeamChangedMessage>([this](const auto& msg) {
        m_teamId = msg.teamId;
        return ListenerResult::Continue;
    })->setPriority(-10000);

    nm.listenGlobal<msg::TeamChangedMessage>([this](const auto& msg) {
        m_teamId = msg.teamId;
        m_teamMembers.clear();
        return ListenerResult::Continue;
    })->setPriority(-10000);

    nm.listenGlobal<msg::TeamMembersMessage>([this](const auto& msg) {
        for (auto member : msg.members) {
            m_teamMembers.push_back(member);
        }

        return ListenerResult::Continue;
    })->setPriority(-10000);
}

$on_mod(Loaded) { RoomManager::get(); }

}