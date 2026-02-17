#pragma once

#include <ui/BasePopup.hpp>
#include <ui/misc/CodeEditor.hpp>
#include <globed/core/data/Messages.hpp>
#include <globed/core/net/MessageListener.hpp>

#include <asp/time/SystemTime.hpp>

namespace globed {

class ScriptLogPanelPopup : public BasePopup {
public:
    static ScriptLogPanelPopup* create();

private:
    CodeEditor* m_editor;
    cocos2d::CCDrawNode* m_drawNode;
    CCNode* m_graphLabelsNode;
    CCNode* m_graphBottomLabels;
    bool m_autoRefresh = true;
    bool m_graphShown = false;
    MessageListener<msg::ScriptLogsMessage> m_listener;

    bool init() override;
    void toggleGraphShown(bool graph);
    void refresh();
    void refreshGraph(const std::deque<std::pair<asp::time::SystemTime, float>>& queue);
};

}