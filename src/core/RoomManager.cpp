#include <globed/core/RoomManager.hpp>
#include <globed/core/SessionId.hpp>
#include <globed/core/data/Messages.hpp>
#include <core/net/NetworkManagerImpl.hpp>

using namespace geode::prelude;

namespace globed {

void RoomManager::joinLevel(int levelId) {
    auto& nm = NetworkManagerImpl::get();
    auto serverId = nm.getPreferredServer();
    if (!serverId) {
        // likely not connected
        return;
    }

    // construct a session ID
    auto id = SessionId::fromParts(*serverId, m_roomId, levelId);
    nm.sendJoinSession(id);
}

void RoomManager::leaveLevel() {
    auto& nm = NetworkManagerImpl::get();
    nm.sendLeaveSession();
}

RoomManager::RoomManager() {
    m_roomId = 0;
    m_roomName = "Global Room";

    NetworkManagerImpl::get().listenGlobal<msg::RoomStateMessage>([this](const auto& msg) {
        if (msg.roomId != m_roomId) {
            m_roomId = msg.roomId;
            m_roomName = msg.roomName;
        }

        return ListenerResult::Continue;
    });
}

}