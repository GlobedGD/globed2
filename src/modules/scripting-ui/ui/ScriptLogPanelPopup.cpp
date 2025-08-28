#include "ScriptLogPanelPopup.hpp"
#include <core/net/NetworkManagerImpl.hpp>
#include <modules/scripting/hooks/GJBaseGameLayer.hpp>
#include <globed/util/assert.hpp>

#include <cue/Util.hpp>
#include <UIBuilder.hpp>
#include <asp/time/Instant.hpp>

using namespace geode::prelude;
using namespace asp::time;

namespace globed {

const CCSize ScriptLogPanelPopup::POPUP_SIZE {460.f, 300.f};
static const CCSize GRAPH_SIZE {400.f, 240.f};


bool ScriptLogPanelPopup::setup() {
    m_editor = Build(CodeEditor::create({400.f, 240.f}))
        .pos(this->fromCenter(0.f, 10.f))
        .parent(m_mainLayer);

    m_drawNode = Build(CCDrawNode::create())
        .anchorPoint(0.5f, 0.5f)
        .pos(this->fromCenter(5.f, 20.f))
        .contentSize(GRAPH_SIZE)
        .parent(m_mainLayer);
    m_drawNode->m_bUseArea = false;

    m_graphLabelsNode = Build<CCNode>::create()
        .anchorPoint(0.5f, 0.5f)
        .pos(this->fromCenter(5.f, 20.f))
        .contentSize(GRAPH_SIZE)
        .parent(m_mainLayer);

    m_graphBottomLabels = Build<CCNode>::create()
        .anchorPoint(0.5f, 0.5f)
        .pos(this->fromCenter(5.f, 15.f))
        .contentSize(GRAPH_SIZE)
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
        .pos(this->fromBottomLeft(21.f, 21.f))
        .collect();
    toggler->toggle(m_autoRefresh);

    Build<CCLabelBMFont>::create("Auto refresh", "bigFont.fnt")
        .scale(0.4f)
        .parent(m_mainLayer)
        .pos(this->fromBottomLeft(42.f, 21.f))
        .anchorPoint(0.f, 0.5f);

    // toggler for switching to graph

    auto swToggler = Build(CCMenuItemExt::createTogglerWithStandardSprites(0.8f, [this](auto toggler) {
        bool on = !toggler->isOn();
        this->toggleGraphShown(on);
    }))
        .parent(m_buttonMenu)
        .pos(this->fromBottomRight(76.f, 21.f))
        .collect();

    Build<CCLabelBMFont>::create("Memory", "bigFont.fnt")
        .scale(0.4f)
        .parent(m_mainLayer)
        .pos(this->fromBottomRight(30.f, 21.f));

    this->refresh();

    return true;
}

void ScriptLogPanelPopup::toggleGraphShown(bool graph) {
    m_graphShown = graph;

    m_drawNode->setVisible(graph);
    m_graphLabelsNode->setVisible(graph);
    m_graphBottomLabels->setVisible(graph);

    m_editor->setVisible(!graph);

    this->refresh();
}

void ScriptLogPanelPopup::refresh() {
    if (auto gjbgl = SCBaseGameLayer::get()) {
        if (!m_graphShown) {
            auto start = Instant::now();

            auto& logs = gjbgl->getLogs();
            std::string joined;

            for (auto& log : logs) {
                joined += log;
                joined += '\n';
            }


            m_editor->setContent(joined);
        } else {
            auto& memLimits = gjbgl->getMemLimitBuffer();
            this->refreshGraph(memLimits);
        }
    }
}

void ScriptLogPanelPopup::refreshGraph(const std::deque<std::pair<asp::time::SystemTime, float>>& queue) {
    m_drawNode->clear();

    CCSize chartSize { GRAPH_SIZE.width - 20.f, GRAPH_SIZE.height - 20.f };
    CCPoint chartBase = { 10.f, 10.f };

    // draw grid

    bool addLabels = m_graphLabelsNode->getChildrenCount() == 0;

    // horizontal lines
    for (size_t i = 0; i <= 10; i++) {
        float ygap = chartSize.height / 10;
        float y = chartBase.y + ygap * i;

        m_drawNode->drawSegment(
            { chartBase.x, y },
            { chartBase.x + chartSize.width, y },
            0.5f,
            ccColor4F {0.5f, 0.5f, 0.5f, 0.35f}
        );

        if (addLabels && i > 0) {
            // add labels
            Build<CCLabelBMFont>::create(fmt::format("{}%", i * 10).c_str(), "bigFont.fnt")
                .scale(0.3f)
                .anchorPoint(1.f, 0.5f)
                .pos(chartBase.x - 4.f, y)
                .parent(m_graphLabelsNode);
        }
    }

    // outline
    m_drawNode->drawRect(
        chartBase - CCSize{2.f, 0.f},
        chartBase + chartSize + CCSize{2.f, 0.f}, { 0.f, 0.f, 0.f, 0.f }, 1.0f, { 0.9f, 0.9f, 0.9f, 1.0f }
    );

    // draw the chart itself

    if (queue.size() < 2) return;

    std::vector<CCPoint> segments;

    size_t i = 0;
    for (auto [time, perc] : queue) {
        perc = std::round(std::clamp(perc * 100.f, 0.f, 100.f)) / 100.f;

        float y = chartBase.y + chartSize.height * perc;

        float xgap = chartSize.width / (float)(queue.size() - 1);
        float x = chartBase.x + xgap * i;

        segments.push_back(CCPoint { x, y });

        i++;
    }

    for (size_t i = 0; i < segments.size() - 1; i++) {
        auto& a = segments[i];
        auto& b = segments[i + 1];
        m_drawNode->drawSegment(a, b, 1.0f, {1.f, 1.f, 0.f, 1.f});
    }

    // draw hints at the bottom
    size_t hintCount = std::min<size_t>(8, queue.size());

    auto now = SystemTime::now();
    auto& [oldestTime, oldestVal] = queue.front();
    auto& [newestTime, newestVal] = queue.back();
    auto wholeDelta = (newestTime - oldestTime).value_or(Duration{});

    m_graphBottomLabels->removeAllChildren();

    for (size_t i = 0; i < hintCount; i++) {
        float xgap = chartSize.width / (float)(hintCount - 1);
        float x = chartBase.x + xgap * i;

        auto estPassed = i * wholeDelta / (hintCount - 1);
        auto estTime = oldestTime + estPassed;
        auto dur = now.durationSince(estTime).value_or(Duration{});

        auto mins = dur.minutes();
        auto secs = dur.seconds() % 60;

        Build<CCLabelBMFont>::create(fmt::format("-{:02}:{:02}", mins, secs).c_str(), "chatFont.fnt")
            .scale(0.7f)
            .pos(x, 0.f)
            .parent(m_graphBottomLabels);

        // // find where to slot it between the entries
        // size_t slotIdx = 0;
        // for (; slotIdx < queue.size() - 1; slotIdx++) {
        //     auto& a = queue[slotIdx];
        //     auto& b = queue[slotIdx + 1];

        //     if (estTime <= b.first) {
        //         break;
        //     }
        // }

        // if (slotIdx == queue.size() - 1) {
        //     slotIdx--;
        // }

        // auto& a = queue[slotIdx];
        // auto& b = queue[slotIdx + 1];
        // auto timeDelta = (b.first - a.first).value_or(Duration::fromMillis(1));
        // float t = (float)estPassed.micros() / timeDelta.micros();
    }
}

}