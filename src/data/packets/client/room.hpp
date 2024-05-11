#pragma once
#include <data/packets/packet.hpp>
#include <data/types/room.hpp>

class CreateRoomPacket : public Packet {
    GLOBED_PACKET(13000, false, false)

    CreateRoomPacket() {}

    static std::shared_ptr<Packet> create() {
        return std::make_shared<CreateRoomPacket>();
    }
};

GLOBED_SERIALIZABLE_STRUCT(CreateRoomPacket, ());

class JoinRoomPacket : public Packet {
    GLOBED_PACKET(13001, false, false)

    JoinRoomPacket() {}
    JoinRoomPacket(uint32_t roomId, uint32_t roomToken) : roomId(roomId), roomToken(roomToken) {}

    static std::shared_ptr<Packet> create(uint32_t roomId, uint32_t roomToken) {
        return std::make_shared<JoinRoomPacket>(roomId, roomToken);
    }

    uint32_t roomId, roomToken;
};

GLOBED_SERIALIZABLE_STRUCT(JoinRoomPacket, (roomId, roomToken));

class LeaveRoomPacket : public Packet {
    GLOBED_PACKET(13002, false, false)

    LeaveRoomPacket() {}

    static std::shared_ptr<Packet> create() {
        return std::make_shared<LeaveRoomPacket>();
    }
};

GLOBED_SERIALIZABLE_STRUCT(LeaveRoomPacket, ());

class RequestRoomPlayerListPacket : public Packet {
    GLOBED_PACKET(13003, false, false)

    RequestRoomPlayerListPacket() {}

    static std::shared_ptr<Packet> create() {
        return std::make_shared<RequestRoomPlayerListPacket>();
    }
};

GLOBED_SERIALIZABLE_STRUCT(RequestRoomPlayerListPacket, ());

class UpdateRoomSettingsPacket : public Packet {
    GLOBED_PACKET(13004, false, false)

    UpdateRoomSettingsPacket() {}
    UpdateRoomSettingsPacket(const RoomSettings& settings) : settings(settings) {}

    static std::shared_ptr<Packet> create(const RoomSettings& settings) {
        return std::make_shared<UpdateRoomSettingsPacket>(settings);
    }

    RoomSettings settings;
};

GLOBED_SERIALIZABLE_STRUCT(UpdateRoomSettingsPacket, (settings));

class RoomSendInvitePacket : public Packet {
    GLOBED_PACKET(13005, false, false)

    RoomSendInvitePacket() {}
    RoomSendInvitePacket(int32_t player) : player(player) {}

    static std::shared_ptr<Packet> create(int32_t player) {
        return std::make_shared<RoomSendInvitePacket>(player);
    }

    int32_t player;
};

GLOBED_SERIALIZABLE_STRUCT(RoomSendInvitePacket, (player));

class RequestRoomListPacket : public Packet {
    GLOBED_PACKET(13006, false, false)

    RequestRoomListPacket() {}

    static std::shared_ptr<Packet> create() {
        return std::make_shared<RequestRoomListPacket>();
    }
};

GLOBED_SERIALIZABLE_STRUCT(RequestRoomListPacket, ());
