#pragma once

#include <globed/prelude.hpp>
#include <ui/BasePopup.hpp>

namespace globed {

class UserPunishmentPopup : public BasePopup<UserPunishmentPopup> {
protected:
    bool setup() override;
    bool initCustomSize(const std::string& reason, int64_t expiresAt, bool isBan);

public:
    geode::SimpleTextArea* m_textArea = nullptr;
    int64_t m_expiresAt = 0;
    bool m_isBan = false;

    static UserPunishmentPopup* create(const std::string& reason, int64_t expiresAt, bool isBan);
};

}
