#pragma once
#include <data/packets/packet.hpp>
#include <data/types/gd.hpp>
#include <data/types/room.hpp>

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

class RequestLevelListPacket : public Packet {
    GLOBED_PACKET(11002, false, false)

    RequestLevelListPacket() {}

    static std::shared_ptr<Packet> create() {
        return std::make_shared<RequestLevelListPacket>();
    }
};

GLOBED_SERIALIZABLE_STRUCT(RequestLevelListPacket, ());

class RequestPlayerCountPacket : public Packet {
    GLOBED_PACKET(11003, false, false)

    RequestPlayerCountPacket() {}
    RequestPlayerCountPacket(std::vector<LevelId>&& levelIds) : levelIds(std::move(levelIds)) {}

    static std::shared_ptr<Packet> create(std::vector<LevelId>&& levelIds) {
        return std::make_shared<RequestPlayerCountPacket>(std::move(levelIds));
    }

    std::vector<LevelId> levelIds;
};

GLOBED_SERIALIZABLE_STRUCT(RequestPlayerCountPacket, (levelIds));
