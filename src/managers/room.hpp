#pragma once
#include <defs/util.hpp>

#include <data/types/room.hpp>
#include <util/sync.hpp>

class RoomManager : public SingletonBase<RoomManager> {
    friend class SingletonBase;
    RoomManager();

public:
    RoomInfo& getInfo();
    uint32_t getId();

    bool isInGlobal();
    bool isInRoom();

    void setInfo(const RoomInfo& info);
    void setGlobal();

private:
    RoomInfo roomInfo;
};