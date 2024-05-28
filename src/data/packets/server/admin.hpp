#pragma once
#include <data/packets/packet.hpp>
#include <data/types/admin.hpp>
#include <data/types/gd.hpp>
#include <data/types/user.hpp>

class AdminAuthSuccessPacket : public Packet {
    GLOBED_PACKET(29000, AdminAuthSuccessPacket, false, false)

    AdminAuthSuccessPacket() {}

    ComputedRole role;
};

GLOBED_SERIALIZABLE_STRUCT(AdminAuthSuccessPacket, (role));

class AdminErrorPacket : public Packet {
    GLOBED_PACKET(29001, AdminErrorPacket, true, false)

    AdminErrorPacket() {}

    std::string message;
};

GLOBED_SERIALIZABLE_STRUCT(AdminErrorPacket, (message));

class AdminUserDataPacket : public Packet {
    GLOBED_PACKET(29002, AdminUserDataPacket, true, false)

    AdminUserDataPacket() {}

    UserEntry userEntry;
    std::optional<PlayerRoomPreviewAccountData> accountData;
};

GLOBED_SERIALIZABLE_STRUCT(AdminUserDataPacket, (userEntry, accountData));

class AdminSuccessMessagePacket : public Packet {
    GLOBED_PACKET(29003, AdminSuccessMessagePacket, false, false)

    AdminSuccessMessagePacket() {}

    std::string message;
};

GLOBED_SERIALIZABLE_STRUCT(AdminSuccessMessagePacket, (message));

class AdminAuthFailedPacket : public Packet {
    GLOBED_PACKET(29004, AdminAuthFailedPacket, false, false)

    AdminAuthFailedPacket() {}
};

GLOBED_SERIALIZABLE_STRUCT(AdminAuthFailedPacket, ());
