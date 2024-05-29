#pragma once
#include <data/packets/packet.hpp>
#include <data/types/room.hpp>

class CreateRoomPacket : public Packet {
    GLOBED_PACKET(13000, CreateRoomPacket, false, false)

    CreateRoomPacket() {}
    CreateRoomPacket(const std::string_view roomName, const std::string_view password, const RoomSettings& settings)
        : roomName(roomName), password(password), settings(settings) {}

    std::string roomName;
    std::string password;
    RoomSettings settings;
};

GLOBED_SERIALIZABLE_STRUCT(CreateRoomPacket, (roomName, password, settings));

class JoinRoomPacket : public Packet {
    GLOBED_PACKET(13001, JoinRoomPacket, false, false)

    JoinRoomPacket() {}
    JoinRoomPacket(uint32_t roomId, const std::string_view password) : roomId(roomId), password(password) {}

    uint32_t roomId;
    std::string password;
};

GLOBED_SERIALIZABLE_STRUCT(JoinRoomPacket, (roomId, password));

class LeaveRoomPacket : public Packet {
    GLOBED_PACKET(13002, LeaveRoomPacket, false, false)

    LeaveRoomPacket() {}
};

GLOBED_SERIALIZABLE_STRUCT(LeaveRoomPacket, ());

class RequestRoomPlayerListPacket : public Packet {
    GLOBED_PACKET(13003, RequestRoomPlayerListPacket, false, false)

    RequestRoomPlayerListPacket() {}
};

GLOBED_SERIALIZABLE_STRUCT(RequestRoomPlayerListPacket, ());

class UpdateRoomSettingsPacket : public Packet {
    GLOBED_PACKET(13004, UpdateRoomSettingsPacket, false, false)

    UpdateRoomSettingsPacket() {}
    UpdateRoomSettingsPacket(const RoomSettings& settings) : settings(settings) {}

    RoomSettings settings;
};

GLOBED_SERIALIZABLE_STRUCT(UpdateRoomSettingsPacket, (settings));

class RoomSendInvitePacket : public Packet {
    GLOBED_PACKET(13005, RoomSendInvitePacket, false, false)

    RoomSendInvitePacket() {}
    RoomSendInvitePacket(int32_t player) : player(player) {}

    int32_t player;
};

GLOBED_SERIALIZABLE_STRUCT(RoomSendInvitePacket, (player));

class RequestRoomListPacket : public Packet {
    GLOBED_PACKET(13006, RequestRoomListPacket, false, false)

    RequestRoomListPacket() {}
};

GLOBED_SERIALIZABLE_STRUCT(RequestRoomListPacket, ());
