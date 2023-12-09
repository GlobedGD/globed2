#pragma once
#include <data/packets/packet.hpp>
#include <data/types/gd.hpp>

class SyncIconsPacket : public Packet {
    GLOBED_PACKET(11000, false)

    GLOBED_PACKET_ENCODE { buf.writeValue(icons); }
    GLOBED_PACKET_DECODE_UNIMPL

    SyncIconsPacket(const PlayerIconData& icons) : icons(icons) {}

    static std::shared_ptr<Packet> create(const PlayerIconData& icons) {
        return std::make_shared<SyncIconsPacket>(icons);
    }

    PlayerIconData icons;
};

class RequestPlayerListPacket : public Packet {
    GLOBED_PACKET(11001, false)

    GLOBED_PACKET_ENCODE {}
    GLOBED_PACKET_DECODE_UNIMPL

    RequestPlayerListPacket() {}

    static std::shared_ptr<Packet> create() {
        return std::make_shared<RequestPlayerListPacket>();
    }
};

class CreateRoomPacket : public Packet {
    GLOBED_PACKET(11002, false)

    GLOBED_PACKET_ENCODE {}
    GLOBED_PACKET_DECODE_UNIMPL

    CreateRoomPacket() {}

    static std::shared_ptr<Packet> create() {
        return std::make_shared<CreateRoomPacket>();
    }
};

class JoinRoomPacket : public Packet {
    GLOBED_PACKET(11003, false)

    GLOBED_PACKET_ENCODE { buf.writeString(roomId); }
    GLOBED_PACKET_DECODE_UNIMPL

    JoinRoomPacket(const std::string& roomId) : roomId(roomId) {}

    static std::shared_ptr<Packet> create(const std::string& roomId) {
        return std::make_shared<JoinRoomPacket>(roomId);
    }

    std::string roomId;
};

class LeaveRoomPacket : public Packet {
    GLOBED_PACKET(11004, false)

    GLOBED_PACKET_ENCODE {}
    GLOBED_PACKET_DECODE_UNIMPL

    LeaveRoomPacket() {}

    static std::shared_ptr<Packet> create() {
        return std::make_shared<LeaveRoomPacket>();
    }
};
