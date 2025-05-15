#pragma once
#include <data/packets/packet.hpp>
#include <data/types/admin.hpp>
#include <data/types/gd.hpp>
#include <data/types/user.hpp>

// 29000 - AdminAuthSuccessPacket
class AdminAuthSuccessPacket : public Packet {
    GLOBED_PACKET(29000, AdminAuthSuccessPacket, false, false)

    AdminAuthSuccessPacket() {}

    ComputedRole role;
};

GLOBED_SERIALIZABLE_STRUCT(AdminAuthSuccessPacket, (role));

// 29001 - AdminErrorPacket
class AdminErrorPacket : public Packet {
    GLOBED_PACKET(29001, AdminErrorPacket, true, false)

    AdminErrorPacket() {}

    std::string message;
};

GLOBED_SERIALIZABLE_STRUCT(AdminErrorPacket, (message));

// 29002 - AdminUserDataPacket
class AdminUserDataPacket : public Packet {
    GLOBED_PACKET(29002, AdminUserDataPacket, true, false)

    AdminUserDataPacket() {}

    UserEntry userEntry;
    std::optional<PlayerRoomPreviewAccountData> accountData;
};

GLOBED_SERIALIZABLE_STRUCT(AdminUserDataPacket, (userEntry, accountData));

// 29003 - AdminSuccessMessagePacket
class AdminSuccessMessagePacket : public Packet {
    GLOBED_PACKET(29003, AdminSuccessMessagePacket, true, false)

    AdminSuccessMessagePacket() {}

    std::string message;
};

GLOBED_SERIALIZABLE_STRUCT(AdminSuccessMessagePacket, (message));

// 29004 - AdminAuthFailedPacket
class AdminAuthFailedPacket : public Packet {
    GLOBED_PACKET(29004, AdminAuthFailedPacket, true, false)

    AdminAuthFailedPacket() {}
};

GLOBED_SERIALIZABLE_STRUCT(AdminAuthFailedPacket, ());

// 29005 - AdminPunishmentHistoryPacket
class AdminPunishmentHistoryPacket : public Packet {
    GLOBED_PACKET(29005, AdminPunishmentHistoryPacket, true, false)

    AdminPunishmentHistoryPacket() {}

    std::vector<UserPunishment> entries;
    std::map<int32_t, std::string> modNameData;
};

GLOBED_SERIALIZABLE_STRUCT(AdminPunishmentHistoryPacket, (entries, modNameData));

// 29006 - AdminSuccessfulUpdatePacket
class AdminSuccessfulUpdatePacket : public Packet {
    GLOBED_PACKET(29006, AdminSuccessfulUpdatePacket, true, false)

    AdminSuccessfulUpdatePacket() {}

    UserEntry userEntry;
};

GLOBED_SERIALIZABLE_STRUCT(AdminSuccessfulUpdatePacket, (userEntry));

// 29007 - AdminReceivedNoticeReplyPacket
class AdminReceivedNoticeReplyPacket : public Packet {
    GLOBED_PACKET(29007, AdminReceivedNoticeReplyPacket, true, false)

    uint32_t replyId;
    uint32_t userId;
    std::string userName;
    std::string adminMsg;
    std::string userReply;
};

GLOBED_SERIALIZABLE_STRUCT(AdminReceivedNoticeReplyPacket, (replyId, userId, userName, adminMsg, userReply));

// 29008 - AdminNoticeRecipientCountPacket
class AdminNoticeRecipientCountPacket : public Packet {
    GLOBED_PACKET(29008, AdminNoticeRecipientCountPacket, true, false)

    uint32_t count;
};

GLOBED_SERIALIZABLE_STRUCT(AdminNoticeRecipientCountPacket, (count));
