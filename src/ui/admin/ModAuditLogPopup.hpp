#pragma once

#include <globed/core/data/Messages.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <ui/BasePopup.hpp>

#include <cue/DropdownNode.hpp>

namespace globed {

class ModAuditLogPopup : public BasePopup<ModAuditLogPopup> {
public:
    static const cocos2d::CCSize POPUP_SIZE;

protected:
    cue::DropdownNode* m_typeDropdown;
    std::string_view m_chosenType = "kick";

    cue::DropdownNode* m_userDropdown;
    int m_chosenUserId = 0;

    std::optional<MessageListener<msg::AdminFetchModsResponseMessage>> m_modsListener;

    bool setup();
    void refetch();
    void populateMods(const std::vector<FetchedMod>& mods);
};

}