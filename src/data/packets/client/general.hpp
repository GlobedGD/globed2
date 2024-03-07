#pragma once
#include <data/packets/packet.hpp>
#include <data/types/gd.hpp>

class SyncIconsPacket : public Packet {
    GLOBED_PACKET(11000, false, false)

    SyncIconsPacket() {}
    SyncIconsPacket(const PlayerIconData& icons) : icons(icons) {}

    static std::shared_ptr<Packet> create(const PlayerIconData& icons) {
        return std::make_shared<SyncIconsPacket>(icons);
    }

    PlayerIconData icons;
};

GLOBED_SERIALIZABLE_STRUCT(SyncIconsPacket, (icons));

class RequestGlobalPlayerListPacket : public Packet {
    GLOBED_PACKET(11001, false, false)

    RequestGlobalPlayerListPacket() {}

    static std::shared_ptr<Packet> create() {
        return std::make_shared<RequestGlobalPlayerListPacket>();
    }
};

GLOBED_SERIALIZABLE_STRUCT(RequestGlobalPlayerListPacket, ());

class CreateRoomPacket : public Packet {
    GLOBED_PACKET(11002, false, false)

    CreateRoomPacket() {}

    static std::shared_ptr<Packet> create() {
        return std::make_shared<CreateRoomPacket>();
    }
};

GLOBED_SERIALIZABLE_STRUCT(CreateRoomPacket, ());

class JoinRoomPacket : public Packet {
    GLOBED_PACKET(11003, false, false)

    JoinRoomPacket() {}
    JoinRoomPacket(uint32_t roomId) : roomId(roomId) {}

    static std::shared_ptr<Packet> create(uint32_t roomId) {
        return std::make_shared<JoinRoomPacket>(roomId);
    }

    uint32_t roomId;
};

GLOBED_SERIALIZABLE_STRUCT(JoinRoomPacket, (roomId));

class LeaveRoomPacket : public Packet {
    GLOBED_PACKET(11004, false, false)

    LeaveRoomPacket() {}

    static std::shared_ptr<Packet> create() {
        return std::make_shared<LeaveRoomPacket>();
    }
};

GLOBED_SERIALIZABLE_STRUCT(LeaveRoomPacket, ());

class RequestRoomPlayerListPacket : public Packet {
    GLOBED_PACKET(11005, false, false)

    RequestRoomPlayerListPacket() {}

    static std::shared_ptr<Packet> create() {
        return std::make_shared<RequestRoomPlayerListPacket>();
    }
};

GLOBED_SERIALIZABLE_STRUCT(RequestRoomPlayerListPacket, ());

class RequestLevelListPacket : public Packet {
    GLOBED_PACKET(11006, false, false)

    RequestLevelListPacket() {}

    static std::shared_ptr<Packet> create() {
        return std::make_shared<RequestLevelListPacket>();
    }
};

GLOBED_SERIALIZABLE_STRUCT(RequestLevelListPacket, ());

class RequestPlayerCountPacket : public Packet {
    GLOBED_PACKET(11007, false, false)

    RequestPlayerCountPacket() {}
    RequestPlayerCountPacket(std::vector<LevelId>&& levelIds) : levelIds(std::move(levelIds)) {}

    static std::shared_ptr<Packet> create(std::vector<LevelId>&& levelIds) {
        return std::make_shared<RequestPlayerCountPacket>(std::move(levelIds));
    }

    std::vector<LevelId> levelIds;
};

GLOBED_SERIALIZABLE_STRUCT(RequestPlayerCountPacket, (levelIds));
