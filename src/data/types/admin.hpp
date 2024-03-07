#pragma once
#include <data/bytebuffer.hpp>
#include <util/data.hpp>

class UserEntry {
public:
    UserEntry() {}
    UserEntry(
        int accountId,
        std::optional<std::string> userName,
        std::optional<std::string> nameColor,
        int userRole,
        bool isBanned,
        bool isMuted,
        bool isWhitelisted,
        std::optional<std::string> adminPassword,
        std::optional<std::string> violationReason,
        std::optional<int64_t> violationExpiry
    ) : accountId(accountId), userName(userName), nameColor(nameColor), userRole(userRole), isBanned(isBanned), isMuted(isMuted), isWhitelisted(isWhitelisted), adminPassword(adminPassword), violationReason(violationReason), violationExpiry(violationExpiry) {}

    int accountId;
    std::optional<std::string> userName;
    std::optional<std::string> nameColor;
    int userRole;
    bool isBanned;
    bool isMuted;
    bool isWhitelisted;
    std::optional<std::string> adminPassword;
    std::optional<std::string> violationReason;
    std::optional<int64_t> violationExpiry;
};

GLOBED_SERIALIZABLE_STRUCT(
    UserEntry,
    (
        accountId,
        userName,
        nameColor,
        userRole,
        isBanned,
        isMuted,
        isWhitelisted,
        adminPassword,
        violationReason,
        violationExpiry
    )
);
