#pragma once
#include <ui/BasePopup.hpp>

#include <cue/ListNode.hpp>

namespace globed {

class GlobedMenuLayer;

class ServerListPopup : public BasePopup {
public:
    void reloadList();
    static ServerListPopup* create(GlobedMenuLayer* menuLayer);

protected:
    friend class BasePopup;

    cue::ListNode* m_list;
    GlobedMenuLayer* m_menuLayer;

    bool init(GlobedMenuLayer*);
};

}
