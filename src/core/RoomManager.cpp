#include <globed/core/RoomManager.hpp>
#include <globed/core/SessionId.hpp>
#include <globed/core/data/Messages.hpp>
#include <core/net/NetworkManagerImpl.hpp>

using namespace geode::prelude;

namespace globed {

void RoomManager::joinLevel(int levelId) {
    auto& nm = NetworkManagerImpl::get();

    // depending on whether we are in a room or not, we need to either get the room's server ID or our preferred

    uint8_t srv;
    if (this->isInGlobal()) {
        if (auto serverId = nm.getPreferredServer()) {
            srv = *serverId;
        } else {
            log::warn("Failed to join level, no servers available");
            return;
        }
    } else {
        srv = m_settings.serverId;
    }

    // construct a session ID
    auto id = SessionId::fromParts(srv, m_roomId, levelId);
    nm.sendJoinSession(id);
}

void RoomManager::leaveLevel() {
    auto& nm = NetworkManagerImpl::get();
    nm.sendLeaveSession();
}

bool RoomManager::isInGlobal() {
    return m_roomId == 0;
}

RoomManager::RoomManager() {
    m_roomId = 0;
    m_roomName = "Global Room";

    auto listener = NetworkManagerImpl::get().listenGlobal<msg::RoomStateMessage>([this](const auto& msg) {
        if (msg.roomId != m_roomId) {
            m_roomId = msg.roomId;
            m_roomName = msg.roomName;
        }

        m_settings = msg.settings;

        return ListenerResult::Continue;
    });
    listener->setPriority(-10000);
}

}