#include "ProfilerOverlay.hpp"

#include <ui/Core.hpp>
#include <UIBuilder.hpp>

using namespace geode::prelude;
using namespace asp::time;

namespace globed {

bool ProfilerOverlay::init(CCSize size) {
    float legendWidth = 80.f;
    m_drawSize = CCSize{size.width - legendWidth - 3.f, size.height};

    m_drawNode = Build<CCDrawNode>::create()
        .contentSize(m_drawSize)
        .anchorPoint(0.f, 0.f)
        .parent(this);
    m_drawNode->m_bUseArea = false;

    m_legend = Build<ColumnContainer>::create()
        .anchorPoint(0.f, 0.f)
        .pos(m_drawSize.width + 3.f, 0.f)
        .parent(this);

    this->setContentSize(size);

    this->redraw();

    return true;
}

void ProfilerOverlay::redraw() {
    m_drawNode->clear();

    // draw background
    CCPoint bgVertices[] = {
        {0.f, 0.f},
        {m_drawSize.width, 0.f},
        {m_drawSize.width, m_drawSize.height},
        {0.f, m_drawSize.height}
    };
    m_drawNode->drawPolygon(bgVertices, 4, {0.f, 0.f, 0.f, 0.3f}, 0, {});

    if (m_frames.empty()) return;

    size_t count = m_frames.size();
    float xstep = m_drawSize.width / static_cast<float>(count);

    float maxTime = asp::iter::from(m_frames)
        .copied()
        .map([](auto pair) { return pair.second.totalTime.template seconds<float>(); })
        .max()
        .value_or(0.f);

    float animInterval = CCDirector::get()->getAnimationInterval();
    float targetMax = std::max(maxTime, animInterval * 0.5f);

    if (targetMax > m_smoothedMaxTime) {
        m_smoothedMaxTime = targetMax;
    } else {
        m_smoothedMaxTime = m_smoothedMaxTime + (targetMax - m_smoothedMaxTime) * 0.05f;
    }

    // draw the target line
    float lineY = (animInterval / m_smoothedMaxTime) * m_drawSize.height;
    if (lineY < m_drawSize.height) {
        m_drawNode->drawSegment(
            {0.f, lineY},
            {m_drawSize.width, lineY},
            0.5f,
            {1.f, 1.f, 1.f, 0.4f}
        );
    }

    // draw the charts
    for (size_t i = 0; i < count; i++) {
        auto& frame = m_frames[i].second;
        float y = 0;
        float x = i * xstep;

        for (const auto& sample : frame.samples) {
            float stime = sample.time.seconds<float>();
            float sheight = (stime / m_smoothedMaxTime) * m_drawSize.height;
            if (stime > 0 && sheight < 0.5f) sheight = 0.5f;

            CCPoint vertices[] = {
                {x, y},
                {x + xstep, y},
                {x + xstep, y + sheight},
                {x, y + sheight}
            };

            m_drawNode->drawPolygon(vertices, 4, sample.color, 1, {0.f, 0.f, 0.f, 0.f});
            y += sheight;
        }
    }
}

void ProfilerOverlay::updateWithFrame(const ProfilerFrame& frame) {
    // log::debug("Add new frame (time {}):", frame.totalTime);
    // for (auto& s : frame.samples) {
    //     log::debug("- {}: {}", s.name, s.time);
    // }

    m_frames.emplace_back(Instant::now(), frame);

    // keep last 3 seconds of data in the graph
    auto cutoff = Instant::now() - Duration::fromSecs(3);

    while (!m_frames.empty()) {
        auto& [time, _] = m_frames.front();
        if (time < cutoff) {
            m_frames.pop_front();
        } else {
            break;
        }
    }

    this->redraw();

    for (auto& sample : frame.samples) {
        if (m_legendNames.insert(sample.name).second) {
            this->addNewEntryToLegend(sample.name, sample.color);
        }
    }
}

void ProfilerOverlay::addNewEntryToLegend(std::string_view name, cocos2d::ccColor4F color) {
    auto container = Build<RowContainer>::create()
        .parent(m_legend)
        .collect();

    Build<CCLayerColor>::create(ccc4BFromccc4F(color))
        .contentSize({5.f, 5.f})
        .parent(container);

    auto label = Build<Label>::create(name, "bigFont.fnt")
        .parent(container)
        .collect();
    label->limitLabelWidth(70.f, 0.5f, 0.1f);

    container->updateLayout();
    m_legend->updateLayout();
}

ProfilerOverlay* ProfilerOverlay::create(CCSize size) {
    auto ret = new ProfilerOverlay();
    if (ret->init(size)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

}
