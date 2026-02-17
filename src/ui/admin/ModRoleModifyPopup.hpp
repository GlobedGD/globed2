#pragma once

#include <globed/core/data/Messages.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <ui/BasePopup.hpp>
#include <ui/misc/LoadingPopup.hpp>

#include <Geode/Geode.hpp>

namespace globed {

class ModRoleModifyPopup : public BasePopup {
public:
    using Callback = geode::Function<void()>;

    static ModRoleModifyPopup* create(int32_t accountId, std::vector<uint8_t> roleIds);
    void setCallback(Callback&& cb);

private:
    int m_accountId = 0;
    std::vector<uint8_t> m_roleIds;
    Callback m_callback;

    MessageListener<msg::AdminResultMessage> m_listener;
    LoadingPopup* m_loadPopup = nullptr;

    bool init(int32_t accountId, std::vector<uint8_t> roleIds);
    void submit();

    void startWaiting();
    void stopWaiting(const msg::AdminResultMessage& msg);
};

}