#pragma once
#include <data/packets/packet.hpp>
#include <data/types/gd.hpp>

class SyncIconsPacket : public Packet {
    GLOBED_PACKET(11000, false)

    GLOBED_PACKET_ENCODE { buf.writeValue(icons); }

    SyncIconsPacket(const PlayerIconData& icons) : icons(icons) {}

    static std::shared_ptr<Packet> create(const PlayerIconData& icons) {
        return std::make_shared<SyncIconsPacket>(icons);
    }

    PlayerIconData icons;
};

class RequestGlobalPlayerListPacket : public Packet {
    GLOBED_PACKET(11001, false)

    GLOBED_PACKET_ENCODE {}

    RequestGlobalPlayerListPacket() {}

    static std::shared_ptr<Packet> create() {
        return std::make_shared<RequestGlobalPlayerListPacket>();
    }
};

class CreateRoomPacket : public Packet {
    GLOBED_PACKET(11002, false)

    GLOBED_PACKET_ENCODE {}

    CreateRoomPacket() {}

    static std::shared_ptr<Packet> create() {
        return std::make_shared<CreateRoomPacket>();
    }
};

class JoinRoomPacket : public Packet {
    GLOBED_PACKET(11003, false)

    GLOBED_PACKET_ENCODE { buf.writeU32(roomId); }

    JoinRoomPacket(uint32_t roomId) : roomId(roomId) {}

    static std::shared_ptr<Packet> create(uint32_t roomId) {
        return std::make_shared<JoinRoomPacket>(roomId);
    }

    uint32_t roomId;
};

class LeaveRoomPacket : public Packet {
    GLOBED_PACKET(11004, false)

    GLOBED_PACKET_ENCODE {}

    LeaveRoomPacket() {}

    static std::shared_ptr<Packet> create() {
        return std::make_shared<LeaveRoomPacket>();
    }
};

class RequestRoomPlayerListPacket : public Packet {
    GLOBED_PACKET(11005, false)

    GLOBED_PACKET_ENCODE {}

    RequestRoomPlayerListPacket() {}

    static std::shared_ptr<Packet> create() {
        return std::make_shared<RequestRoomPlayerListPacket>();
    }
};