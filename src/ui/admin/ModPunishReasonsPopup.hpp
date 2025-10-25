#pragma once

#include <globed/prelude.hpp>
#include <globed/core/data/UserPunishment.hpp>
#include <ui/BasePopup.hpp>

namespace globed {

class ModPunishReasonsPopup : public BasePopup<ModPunishReasonsPopup, UserPunishmentType> {
public:
    static constexpr CCSize POPUP_SIZE { 400.f, 260.f };
    using Callback = std23::move_only_function<void(const std::string&)>;

    inline void setCallback(Callback&& cb) {
        m_callback = std::move(cb);
    }

    inline void invokeCallback(const std::string& reason) {
        if (m_callback) {
            m_callback(reason);
        }

        this->onClose(nullptr);
    }

private:
    Callback m_callback;

    bool setup(UserPunishmentType type) override;
};

}
