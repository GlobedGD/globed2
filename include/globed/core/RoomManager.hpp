#pragma once

#include <globed/util/singleton.hpp>
#include <globed/core/data/RoomSettings.hpp>

namespace globed {

/// Class that manages client's current room state.
/// It also manages joining and leaving levels.
class RoomManager : public SingletonLeakBase<RoomManager> {
public:
    void joinLevel(int levelId);
    void leaveLevel();

    bool isInGlobal();
    uint32_t getRoomId();
    std::optional<uint8_t> pickServerId();

private:
    friend class SingletonLeakBase;
    RoomManager();
    ~RoomManager() = default;

    uint32_t m_roomId = 0;
    std::string m_roomName;
    RoomSettings m_settings{};
};

}