#pragma once

#include <globed/core/data/Messages.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <ui/BasePopup.hpp>
#include <ui/misc/LoadingPopup.hpp>

#include <Geode/Geode.hpp>
#include <std23/move_only_function.h>

namespace globed {

class ModRoleModifyPopup : public BasePopup<ModRoleModifyPopup, int32_t, std::vector<uint8_t>> {
public:
    using Callback = std23::move_only_function<void()>;

    static cocos2d::CCSize POPUP_SIZE;

    static ModRoleModifyPopup *create(int32_t accountId, std::vector<uint8_t> roleIds);
    void setCallback(Callback &&cb);

private:
    int m_accountId = 0;
    std::vector<uint8_t> m_roleIds;
    Callback m_callback;

    std::optional<MessageListener<msg::AdminResultMessage>> m_listener;
    LoadingPopup *m_loadPopup = nullptr;

    bool setup(int32_t accountId, std::vector<uint8_t> roleIds) override;
    void submit();

    void startWaiting();
    void stopWaiting(const msg::AdminResultMessage &msg);
};

} // namespace globed