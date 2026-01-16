#pragma once
#include <ui/BasePopup.hpp>

#include <cue/ListNode.hpp>

namespace globed {

class GlobedMenuLayer;

class ServerListPopup : public BasePopup<ServerListPopup, GlobedMenuLayer *> {
public:
    void reloadList();

protected:
    friend class BasePopup;
    static const cocos2d::CCSize POPUP_SIZE;

    cue::ListNode *m_list;
    GlobedMenuLayer *m_menuLayer;

    bool setup(GlobedMenuLayer *) override;
};

} // namespace globed
