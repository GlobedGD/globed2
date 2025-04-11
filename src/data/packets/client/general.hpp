#pragma once
#include <data/packets/packet.hpp>
#include <data/types/gd.hpp>
#include <data/types/room.hpp>
#include <data/types/user.hpp>

// 11000 - SyncIconsPacket
class SyncIconsPacket : public Packet {
    GLOBED_PACKET(11000, SyncIconsPacket, false, false)

    SyncIconsPacket() {}
    SyncIconsPacket(const PlayerIconData& icons) : icons(icons) {}

    PlayerIconData icons;
};

GLOBED_SERIALIZABLE_STRUCT(SyncIconsPacket, (icons));

// 11001 - RequestGlobalPlayerListPacket
class RequestGlobalPlayerListPacket : public Packet {
    GLOBED_PACKET(11001, RequestGlobalPlayerListPacket, false, false)

    RequestGlobalPlayerListPacket() {}
};

GLOBED_SERIALIZABLE_STRUCT(RequestGlobalPlayerListPacket, ());

// 11002 - RequestLevelListPacket
class RequestLevelListPacket : public Packet {
    GLOBED_PACKET(11002, RequestLevelListPacket, false, false)

    RequestLevelListPacket() {}
};

GLOBED_SERIALIZABLE_STRUCT(RequestLevelListPacket, ());

// 11003 - RequestPlayerCountPacket
class RequestPlayerCountPacket : public Packet {
    GLOBED_PACKET(11003, RequestPlayerCountPacket, false, false)

    RequestPlayerCountPacket() {}
    RequestPlayerCountPacket(std::vector<LevelId>&& levelIds) : levelIds(std::move(levelIds)) {}

    std::vector<LevelId> levelIds;
};

GLOBED_SERIALIZABLE_STRUCT(RequestPlayerCountPacket, (levelIds));

// 11004 - UpdatePlayerStatusPacket
class UpdatePlayerStatusPacket : public Packet {
    GLOBED_PACKET(11004, UpdatePlayerStatusPacket, false, false);

    UpdatePlayerStatusPacket() {}
    UpdatePlayerStatusPacket(const UserPrivacyFlags& flags) : flags(flags) {}

    UserPrivacyFlags flags;
};

GLOBED_SERIALIZABLE_STRUCT(UpdatePlayerStatusPacket, (flags));

// 11005 - LinkCodeRequestPacket
class LinkCodeRequestPacket : public Packet {
    GLOBED_PACKET(11005, LinkCodeRequestPacket, false, false);

    LinkCodeRequestPacket() {}
};

GLOBED_SERIALIZABLE_STRUCT(LinkCodeRequestPacket, ());

// 11006 - NoticeReplyPacket
class NoticeReplyPacket : public Packet {
    GLOBED_PACKET(11006, NoticeReplyPacket, false, false);

    NoticeReplyPacket() {}
    NoticeReplyPacket(uint32_t replyId, std::string message) : replyId(replyId), message(std::move(message)) {}

    uint32_t replyId;
    std::string message;
};

GLOBED_SERIALIZABLE_STRUCT(NoticeReplyPacket, (replyId, message));

// 11007 - RequestMotdPacket
class RequestMotdPacket : public Packet {
    GLOBED_PACKET(11007, RequestMotdPacket, false, true)

    RequestMotdPacket() {}
    RequestMotdPacket(std::string motdHash, bool expectResponse = false) : motdHash(std::move(motdHash)), expectResponse(expectResponse) {}

    std::string motdHash;
    bool expectResponse;
};

GLOBED_SERIALIZABLE_STRUCT(RequestMotdPacket, (motdHash, expectResponse));
