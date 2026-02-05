#pragma once

#include <globed/prelude.hpp>
#include <globed/core/data/UserPunishment.hpp>
#include <ui/BasePopup.hpp>

#include <Geode/utils/function.hpp>
#include <cue/ListNode.hpp>

namespace globed {

class ModPunishReasonsPopup : public BasePopup {
public:
    static ModPunishReasonsPopup* create(UserPunishmentType type);
    using Callback = geode::Function<void(const std::string&)>;

    inline void setCallback(Callback&& cb) {
        m_callback = std::move(cb);
    }

    inline void invokeCallback(const std::string& reason) {
        if (m_callback) {
            m_callback(reason);
        }

        this->onClose(nullptr);
    }

    void commitNewReason(std::string reason);
    void commitEditReason(size_t index, std::string reason);
    void commitDeleteReason(size_t index);
    void openEditReasonPopup(size_t index, const std::string& currentReason);
    void deleteCustomReason(size_t index);

private:
    Callback m_callback;
    cue::ListNode* m_serverList;
    cue::ListNode* m_customList;
    UserPunishmentType m_type;

    bool init(UserPunishmentType type);
    void toggleCustomReasons(bool custom);
    cue::ListNode* makeList(std::span<std::string> reasons, bool custom);
    std::vector<std::string> getCustomReasons();
    void saveCustomReasons(const std::vector<std::string>& reasons);

    void createBlankReason();
};

}
