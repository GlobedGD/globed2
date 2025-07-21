#pragma once

#include <globed/util/singleton.hpp>

namespace globed {

/// Class that manages client's current room state.
/// It also manages joining and leaving levels.
class RoomManager : public SingletonBase<RoomManager> {
public:
    void joinLevel(int levelId);
    void leaveLevel();

private:
    friend class SingletonBase;
    RoomManager() = default;
    ~RoomManager() = default;

    uint32_t m_roomId = 0;
};

}