#include <globed/core/RoomManager.hpp>
#include <globed/core/SessionId.hpp>
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
    nm.joinSession(id);
}

void RoomManager::leaveLevel() {
    auto& nm = NetworkManagerImpl::get();
    nm.leaveSession();
}


}