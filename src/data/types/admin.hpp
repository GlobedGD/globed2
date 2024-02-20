#pragma once
#include <data/bytebuffer.hpp>
#include <util/data.hpp>

class UserEntry {
public:
    UserEntry() {}
    UserEntry(
        int accountId,
        std::optional<std::string> nameColor,
        int userRole,
        bool isBanned,
        bool isMuted,
        bool isWhitelisted,
        std::optional<std::string> adminPassword,
        std::optional<std::string> violationReason,
        std::optional<int64_t> violationExpiry
    ) : accountId(accountId), nameColor(nameColor), userRole(userRole), isBanned(isBanned), isMuted(isMuted), isWhitelisted(isWhitelisted), adminPassword(adminPassword), violationReason(violationReason), violationExpiry(violationExpiry) {}

    GLOBED_ENCODE {
        buf.writeI32(accountId);
        buf.writeBool(nameColor.has_value());
        if (nameColor.has_value()) {
            buf.writeString(nameColor.value());
        }

        buf.writeI32(userRole);
        buf.writeBool(isBanned);
        buf.writeBool(isMuted);
        buf.writeBool(isWhitelisted);

        buf.writeBool(adminPassword.has_value());
        if (adminPassword.has_value()) {
            buf.writeString(adminPassword.value());
        }

        buf.writeBool(violationReason.has_value());
        if (violationReason.has_value()) {
            buf.writeString(violationReason.value());
        }

        buf.writeBool(violationExpiry.has_value());
        if (violationExpiry.has_value()) {
            buf.writeI64(violationExpiry.value());
        }
    }

    GLOBED_DECODE {
        accountId = buf.readI32();
        if (buf.readBool()) {
            nameColor = buf.readString();
        }

        userRole = buf.readI32();
        isBanned = buf.readBool();
        isMuted = buf.readBool();
        isWhitelisted = buf.readBool();

        if (buf.readBool()) {
            adminPassword = buf.readString();
        }

        if (buf.readBool()) {
            violationReason = buf.readString();
        }

        if (buf.readBool()) {
            violationExpiry = buf.readI64();
        }
    }

    int accountId;
    std::optional<std::string> nameColor;
    int userRole;
    bool isBanned;
    bool isMuted;
    bool isWhitelisted;
    std::optional<std::string> adminPassword;
    std::optional<std::string> violationReason;
    std::optional<int64_t> violationExpiry;
};
