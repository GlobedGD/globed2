#pragma once
#include <data/packets/packet.hpp>
#include <data/types/admin.hpp>
#include <data/types/gd.hpp>

class AdminAuthSuccessPacket : public Packet {
    GLOBED_PACKET(29000, false, false)
    GLOBED_PACKET_DECODE {
        role = buf.readI32();
    }

    int role;
};

class AdminErrorPacket : public Packet {
    GLOBED_PACKET(29001, true, false)
    GLOBED_PACKET_DECODE {
        message = buf.readString();
    }

    std::string message;
};

class AdminUserDataPacket : public Packet {
    GLOBED_PACKET(29002, true, false)
    GLOBED_PACKET_DECODE {
        userEntry = buf.readValue<UserEntry>();
        accountData = buf.readOptionalValue<PlayerRoomPreviewAccountData>();
    }

    UserEntry userEntry;
    std::optional<PlayerRoomPreviewAccountData> accountData;
};

class AdminSuccessMessagePacket : public Packet {
    GLOBED_PACKET(29003, false, false)
    GLOBED_PACKET_DECODE {
        message = buf.readString();
    }

    std::string message;
};

class AdminAuthFailedPacket : public Packet {
    GLOBED_PACKET(29004, true, false)
    GLOBED_PACKET_DECODE {}
};
