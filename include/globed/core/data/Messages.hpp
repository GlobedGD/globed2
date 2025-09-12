/// This file contains all the messages that can be listened to with `NetworkManager`.

#pragma once
#include "AdminLogs.hpp"
#include "PlayerState.hpp"
#include "PlayerDisplayData.hpp"
#include "RoomListingInfo.hpp"
#include "UserRole.hpp"
#include "Event.hpp"
#include "RoomTeam.hpp"
#include "UserPunishment.hpp"
#include "FetchedMod.hpp"
#include "Credits.hpp"
#include "ModPermissions.hpp"

namespace globed::msg {

// Login

struct CentralLoginOkMessage {
    std::string newToken;
    std::vector<UserRole> allRoles;
    std::vector<UserRole> userRoles;
    std::optional<MultiColor> nameColor;
    ModPermissions perms;
};

// Rooms

struct RoomStateMessage {
    uint32_t roomId;
    int32_t roomOwner;
    std::string roomName;
    std::vector<RoomPlayer> players;
    RoomSettings settings;
    std::vector<RoomTeam> teams;
};

struct RoomPlayersMessage {
    std::vector<RoomPlayer> players;
};

enum class RoomJoinFailedReason : uint16_t {
    NotFound = 0,
    InvalidPasscode,
    Full,
    Banned,

    Last_ = Banned,
};

struct RoomJoinFailedMessage {
    RoomJoinFailedReason reason;
};

enum class RoomCreateFailedReason : uint16_t {
    InvalidName = 0,
    InvalidSettings,
    InvalidPasscode,
    InvalidServer,
    ServerDown,
    InappropriateName,

    Last_ = InappropriateName,
};

struct RoomCreateFailedMessage {
    RoomCreateFailedReason reason;
};

struct RoomListMessage {
    std::vector<RoomListingInfo> rooms;
};

struct TeamCreationResultMessage {
    bool success;
    uint16_t teamCount;
};

struct TeamChangedMessage {
    uint16_t teamId;
};

struct TeamMembersMessage {
    std::vector<std::pair<int, uint16_t>> members;
};

struct TeamsUpdatedMessage {
    std::vector<RoomTeam> teams;
};

struct RoomSettingsUpdatedMessage {
    RoomSettings settings;
};

struct InvitedMessage {
    PlayerAccountData invitedBy;
    uint64_t token;
};

struct InviteTokenCreatedMessage {
    uint64_t token;
};

// Misc

struct WarpPlayerMessage {
    SessionId sessionId;
};

struct NoticeMessage {
    int senderId;
    std::string senderName;
    std::string message;
    bool canReply;
};

struct PlayerCountsMessage {
    std::vector<std::pair<SessionId, uint16_t>> counts;
};

struct GlobalPlayersMessage {
    std::vector<MinimalRoomPlayer> players;
};

struct LevelListMessage {
    std::vector<std::pair<SessionId, uint16_t>> levels;
};

// Level data

struct LevelDataMessage {
    std::vector<PlayerState> players;
    std::vector<PlayerDisplayData> displayDatas;
    std::vector<InEvent> events;
};

// Script logs

struct ScriptLogsMessage {
    std::vector<std::string> logs;
    float memUsage;
};

// Credits

struct CreditsMessage {
    std::vector<CreditsCategory> categories;
    bool unavailable;
};

// Discord link state

struct DiscordLinkStateMessage {
    uint64_t id;
    std::string username;
    std::string avatarUrl;
};

// Discord link attempt

struct DiscordLinkAttemptMessage {
    uint64_t id;
    std::string username;
    std::string avatarUrl;
};

// User data changed

struct UserDataChangedMessage {
    std::vector<uint8_t> roles;
    std::optional<MultiColor> nameColor;
    ModPermissions perms;
};

// Admin

struct AdminResultMessage {
    bool success;
    std::string error;
};

struct AdminFetchResponseMessage {
    int accountId;
    bool found;
    bool whitelisted;
    std::vector<uint8_t> roles;
    std::optional<UserPunishment> activeBan;
    std::optional<UserPunishment> activeRoomBan;
    std::optional<UserPunishment> activeMute;
    size_t punishmentCount;
};

struct AdminFetchModsResponseMessage {
    std::vector<FetchedMod> users;
};

struct AdminLogsResponseMessage {
    std::vector<AdminAuditLog> logs;
    std::vector<PlayerAccountData> users;
};

}
