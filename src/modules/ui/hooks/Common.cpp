#include "Common.hpp"
#include <globed/core/RoomManager.hpp>
#include <globed/core/PopupManager.hpp>
#include <core/net/NetworkManagerImpl.hpp>

using namespace geode::prelude;

namespace globed {

bool disallowLevelJoin() {
    // don't allow joining when in a follower room
    auto& rm = RoomManager::get();
    if (!rm.isInFollowerRoom() || rm.isOwner()) return false;

    globed::alert(
        "Warning",
        "You are in a <cy>follower room</c>, only the <cg>room host</c> can choose what levels to play. "
        "To be able to join levels or open the level editor, you need to <cr>leave the room</c>."
    );

    return true;
}

}
