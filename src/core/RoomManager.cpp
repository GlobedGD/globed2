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

uint32_t RoomManager::getRoomId() {
    return m_roomId;
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

RoomManager::RoomManager() {
    m_roomId = 0;
    m_roomName = "Global Room";

    auto listener = NetworkManagerImpl::get().listenGlobal<msg::RoomStateMessage>([this](const auto& msg) {
        if (msg.roomId != m_roomId) {
            m_roomId = msg.roomId;
            m_roomName = msg.roomName;

            // a change in rooms resets the warp context as well
            globed::_clearWarpContext();
        }

        m_settings = msg.settings;

        return ListenerResult::Continue;
    });
    listener->setPriority(-10000);
}

}