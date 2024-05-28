#pragma once
#include <data/packets/packet.hpp>
#include <data/types/gd.hpp>
#include <data/types/room.hpp>

class SyncIconsPacket : public Packet {
    GLOBED_PACKET(11000, SyncIconsPacket, false, false)

    SyncIconsPacket() {}
    SyncIconsPacket(const PlayerIconData& icons) : icons(icons) {}

    PlayerIconData icons;
};

GLOBED_SERIALIZABLE_STRUCT(SyncIconsPacket, (icons));

class RequestGlobalPlayerListPacket : public Packet {
    GLOBED_PACKET(11001, RequestGlobalPlayerListPacket, false, false)

    RequestGlobalPlayerListPacket() {}
};

GLOBED_SERIALIZABLE_STRUCT(RequestGlobalPlayerListPacket, ());

class RequestLevelListPacket : public Packet {
    GLOBED_PACKET(11002, RequestLevelListPacket, false, false)

    RequestLevelListPacket() {}
};

GLOBED_SERIALIZABLE_STRUCT(RequestLevelListPacket, ());

class RequestPlayerCountPacket : public Packet {
    GLOBED_PACKET(11003, RequestPlayerCountPacket, false, false)

    RequestPlayerCountPacket() {}
    RequestPlayerCountPacket(std::vector<LevelId>&& levelIds) : levelIds(std::move(levelIds)) {}

    std::vector<LevelId> levelIds;
};

GLOBED_SERIALIZABLE_STRUCT(RequestPlayerCountPacket, (levelIds));
