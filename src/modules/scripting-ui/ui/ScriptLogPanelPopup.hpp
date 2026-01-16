#pragma once

#include <globed/core/data/Messages.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <ui/BasePopup.hpp>
#include <ui/misc/CodeEditor.hpp>

#include <asp/time/SystemTime.hpp>

namespace globed {

class ScriptLogPanelPopup : public BasePopup<ScriptLogPanelPopup> {
public:
    static const cocos2d::CCSize POPUP_SIZE;

private:
    CodeEditor *m_editor;
    cocos2d::CCDrawNode *m_drawNode;
    CCNode *m_graphLabelsNode;
    CCNode *m_graphBottomLabels;
    bool m_autoRefresh = true;
    bool m_graphShown = false;
    std::optional<MessageListener<msg::ScriptLogsMessage>> m_listener;

    bool setup() override;
    void toggleGraphShown(bool graph);
    void refresh();
    void refreshGraph(const std::deque<std::pair<asp::time::SystemTime, float>> &queue);
};

} // namespace globed