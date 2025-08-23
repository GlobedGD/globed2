#pragma once

#include <ui/BasePopup.hpp>
#include <ui/misc/CodeEditor.hpp>
#include <globed/core/data/Messages.hpp>
#include <globed/core/net/MessageListener.hpp>

namespace globed {

class ScriptLogPanelPopup : public BasePopup<ScriptLogPanelPopup> {
public:
    static const cocos2d::CCSize POPUP_SIZE;

private:
    CodeEditor* m_editor;
    bool m_autoRefresh = true;
    std::optional<MessageListener<msg::ScriptLogsMessage>> m_listener;

    bool setup() override;
    void refresh();
};

}