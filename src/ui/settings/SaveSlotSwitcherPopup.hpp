#pragma once

#include <globed/prelude.hpp>
#include <ui/BasePopup.hpp>

#include <Geode/utils/function.hpp>
#include <cue/ListNode.hpp>

namespace globed {

class SaveSlotSwitcherPopup : public BasePopup {
public:
    using Callback = geode::Function<void()>;

    static SaveSlotSwitcherPopup* create();

    inline void setSwitchCallback(Callback&& callback) {
        m_callback = std::move(callback);
    }

    void refreshList(bool scrollToTop = false);

    void invokeSwitchCallback();

private:
    Callback m_callback;
    cue::ListNode* m_list;

    bool init();
};

}
