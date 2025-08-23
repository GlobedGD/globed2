#include "ScriptLogPanelPopup.hpp"
#include <core/net/NetworkManagerImpl.hpp>

#include <modules/scripting/hooks/GJBaseGameLayer.hpp>
#include <cue/Util.hpp>
#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

const CCSize ScriptLogPanelPopup::POPUP_SIZE {460.f, 300.f};

bool ScriptLogPanelPopup::setup() {
    m_editor = Build(CodeEditor::create({400.f, 240.f}))
        .pos(this->fromCenter(0.f, 10.f))
        .parent(m_mainLayer);

    m_listener = NetworkManagerImpl::get().listen<msg::ScriptLogsMessage>([this](const auto& msg) {
        if (m_autoRefresh) {
            this->refresh();
        }

        return ListenerResult::Continue;
    });
    (*m_listener)->setPriority(1000); // be after the gjbgl listener

    auto toggler = Build(CCMenuItemExt::createTogglerWithStandardSprites(0.8f, [this](auto toggler) {
        bool on = !toggler->isOn();
        m_autoRefresh = on;
    }))
        .parent(m_buttonMenu)
        .pos(this->fromBottomLeft(24.f, 24.f))
        .collect();
    toggler->toggle(m_autoRefresh);

    Build<CCLabelBMFont>::create("Auto refresh", "bigFont.fnt")
        .scale(0.4f)
        .parent(m_mainLayer)
        .pos(this->fromBottomLeft(44.f, 24.f))
        .anchorPoint(0.f, 0.5f);

    this->refresh();

    return true;
}

void ScriptLogPanelPopup::refresh() {
    if (auto gjbgl = SCBaseGameLayer::get()) {
        auto logs = gjbgl->getLogs();
        std::string joined;

        for (auto& log : logs) {
            joined += log;
            joined += '\n';
        }

        m_editor->setContent(joined);
    }
}

}