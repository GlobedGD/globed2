/// This file contains all the messages that can be listened to with `NetworkManager`.

#pragma once
#include "PlayerState.hpp"
#include "PlayerDisplayData.hpp"
#include "RoomListingInfo.hpp"
#include "UserRole.hpp"
#include "Event.hpp"
#include "RoomTeam.hpp"
#include "UserPunishment.hpp"
#include "FetchedMod.hpp"

namespace globed::msg {

// Login

struct CentralLoginOkMessage {
    std::string newToken;
    std::vector<UserRole> allRoles;
    std::vector<UserRole> userRoles;
    bool isModerator;
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

enum class RoomJoinFailedReason : uint16_t {
    NotFound = 0,
    InvalidPasscode,
    Full,
    Banned,

    Last_ = Full,
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

}
