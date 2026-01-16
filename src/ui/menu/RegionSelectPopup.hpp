#pragma once
#include <ui/BasePopup.hpp>

#include <cue/ListNode.hpp>

namespace globed {

class RegionSelectPopup : public BasePopup<RegionSelectPopup> {
public:
    static const cocos2d::CCSize POPUP_SIZE;

    static void showInfo();
    bool reloadList();

protected:
    cue::ListNode *m_list;

    bool setup();
    void softRefresh(float);
};

} // namespace globed
