#pragma once
#include <ui/BasePopup.hpp>

#include <cue/ListNode.hpp>

namespace globed {

class RegionSelectPopup : public BasePopup {
public:
    static RegionSelectPopup* create();

    static void showInfo();
    bool reloadList();

protected:
    cue::ListNode* m_list;

    bool init();
    void softRefresh(float);
};

}
