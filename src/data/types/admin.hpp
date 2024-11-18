#pragma once
#include <data/bytebuffer.hpp>
#include <util/data.hpp>
#include "user.hpp"

class UserEntry {
public:
    UserEntry() {}
    UserEntry(
        int accountId,
        std::optional<std::string> userName,
        std::optional<std::string> nameColor,
        const std::vector<std::string>& userRoles,
        bool isWhitelisted,
        std::optional<UserPunishment> activeBan,
        std::optional<UserPunishment> activeMute
    ) : accountId(accountId), userName(userName), nameColor(nameColor), userRoles(userRoles), isWhitelisted(isWhitelisted), activeBan(activeBan), activeMute(activeMute) {}

    int accountId;
    std::optional<std::string> userName;
    std::optional<std::string> nameColor;
    std::vector<std::string> userRoles;
    bool isWhitelisted;
    std::optional<UserPunishment> activeBan;
    std::optional<UserPunishment> activeMute;
};

GLOBED_SERIALIZABLE_STRUCT(
    UserEntry,
    (
        accountId,
        userName,
        nameColor,
        userRoles,
        isWhitelisted,
        activeBan,
        activeMute
    )
);
