#pragma once
#include <data/packets/packet.hpp>
#include <data/types/admin.hpp>

class AdminAuthSuccessPacket : public Packet {
    GLOBED_PACKET(29000, false, false)
    GLOBED_PACKET_DECODE {}
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
    }

    UserEntry userEntry;
};

class AdminSuccessMessagePacket : public Packet {
    GLOBED_PACKET(29003, true, false)
    GLOBED_PACKET_DECODE {
        message = buf.readString();
    }

    std::string message;
};
