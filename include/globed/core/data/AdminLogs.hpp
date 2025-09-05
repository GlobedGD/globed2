#pragma once

namespace globed {

struct FetchLogsFilters {
    int issuer = 0;
    int target = 0;
    std::string_view type;
    int64_t before = 0;
    int64_t after = 0;
    uint32_t page = 0;
};

struct AdminAuditLogUser {
    int accountId;
    int userId;
    std::string username;
};

struct AdminAuditLog {
    int id;
    int accountId;
    int targetId;
    std::string type;
    int64_t timestamp;
    int64_t expiresAt;
    std::string message;
};


}