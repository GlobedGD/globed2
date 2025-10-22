#pragma once

#include <globed/prelude.hpp>
#include <ui/BasePopup.hpp>

#include <std23/move_only_function.h>
#include <cue/ListNode.hpp>

namespace globed {

class SaveSlotSwitcherPopup : public BasePopup<SaveSlotSwitcherPopup> {
public:
    static const CCSize POPUP_SIZE;

    using Callback = std23::move_only_function<void()>;

    inline void setSwitchCallback(Callback&& callback) {
        m_callback = std::move(callback);
    }

    void refreshList(bool scrollToTop = false);

    void invokeSwitchCallback();

private:
    Callback m_callback;
    cue::ListNode* m_list;

    bool setup() override;
};

}
