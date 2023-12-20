#pragma once
#include <data/packets/packet.hpp>

class AdminAuthSuccessPacket : public Packet {
    GLOBED_PACKET(29000, false)
    GLOBED_PACKET_DECODE {}
};
