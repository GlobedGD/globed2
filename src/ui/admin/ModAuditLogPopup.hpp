#pragma once

#include <globed/core/data/Messages.hpp>
#include <globed/core/data/AdminLogs.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <ui/BasePopup.hpp>

#include <cue/DropdownNode.hpp>
#include <cue/ListNode.hpp>
#include <cue/LoadingCircle.hpp>

namespace globed {

class ModAuditLogPopup : public BasePopup<ModAuditLogPopup> {
public:
    static const cocos2d::CCSize POPUP_SIZE;

protected:
    cue::DropdownNode* m_typeDropdown;
    cue::DropdownNode* m_userDropdown;
    cue::ListNode* m_list;
    cue::LoadingCircle* m_loadingCircle;
    size_t m_loadReqs = 0;

    FetchLogsFilters m_filters;
    std::optional<MessageListener<msg::AdminFetchModsResponseMessage>> m_modsListener;
    std::optional<MessageListener<msg::AdminLogsResponseMessage>> m_logsListener;

    bool setup();
    void refetch();

    void populateMods(const std::vector<FetchedMod>& mods);
    void populateLogs(const std::vector<AdminAuditLog>& logs, const std::vector<PlayerAccountData>& users);
};

}