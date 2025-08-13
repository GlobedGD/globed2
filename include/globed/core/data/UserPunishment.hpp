#pragma once
#include <string>
#include <stdint.h>

namespace globed {

struct UserPunishment {
    int32_t issuedBy;
    int64_t issuedAt;
    std::string reason;
    int64_t expiresAt;
};

}
