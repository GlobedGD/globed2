#include "SettingsPopup.hpp"
#include <core/net/NetworkManagerImpl.hpp>
#include <ui/Core.hpp>

#include <cue/LoadingCircle.hpp>
#include <cue/Util.hpp>

using namespace geode::prelude;

namespace globed {

bool APSSettingsPopup::init(APSPlayLayer* layer) {
    if (!Popup::init(340.f, 200.f)) return false;

    this->setTitle("Switcheroo Settings");

    m_settings = layer->m_fields->m_controller.m_settings;
    m_layer = layer;

    m_btn = Build(ButtonSprite::create("Start", "bigFont.fnt", "GJ_button_01.png", 0.8f))
        .intoMenuItem([this] {
            m_btn->removeFromParent();
            this->startGame();
        })
        .pos(this->fromBottom(40.f))
        .parent(m_buttonMenu)
        .collect();

    // left side - radio buttons for algorithm
    auto llayout = Build(ColumnContainer::create())
        .parent(m_mainLayer)
        .pos(this->fromCenter(-100.f, 20.f))
        .collect();

    std::array<std::pair<const char*, APSSelectionAlgorithm>, 3> algos = {
        std::make_pair("Random", APSSelectionAlgorithm::Random),
        std::make_pair("Fair Random", APSSelectionAlgorithm::FairRandom),
        std::make_pair("Sequential", APSSelectionAlgorithm::Sequential),
    };

    for (auto& [name, algo] : algos) {}

    // right side - sliders for interval, var, warning delay

    auto rlayout = Build(ColumnContainer::create())
        .parent(m_mainLayer)
        .pos(this->fromCenter(100.f, 20.f))
        .collect();

    std::array<std::pair<const char*, float*>, 3> sliders = {
        std::make_pair("Switch Interval", &m_settings.m_interval),
        std::make_pair("Switch Variance", &m_settings.m_intervalVar),
        std::make_pair("Warning Delay", &m_settings.m_warningDelay),
    };

    for (auto& [name, field] : sliders) {
        auto cont = Build<ColumnContainer>::create()
            .parent(rlayout)
            .collect();

        Build(createSlider())
            .with([&] (cue::Slider* slider) {
                slider->setCallback([this, field](cue::Slider*, double value) {
                    *field = value;
                    m_dirty = true;
                });
            })
            .parent(cont);

        cont->updateLayout();
    }

    rlayout->updateLayout();
    cue::attachBackground(rlayout);

    return true;
}

void APSSettingsPopup::onClose(CCObject*) {
    this->apply();
    Popup::onClose(nullptr);
}

void APSSettingsPopup::startGame() {
    m_dirty = false;

    m_layer->updateSettings(m_settings);
    m_layer->restartSwitchCycle();

    // wait until we receive back the event from the server, then close this popup
    auto circle = Build<cue::LoadingCircle>::create(true)
        .scale(0.7f)
        .parent(m_mainLayer)
        .pos(this->fromBottom(48.f));

    m_buttonMenu->setEnabled(false);

    m_listener = NetworkManagerImpl::get().listen<msg::LevelDataMessage>([this](const msg::LevelDataMessage& msg) {
        for (auto& event : msg.events) {
            if (event.is<SwitcherooFullStateEvent>()) {
                this->onClose(nullptr);
            }
        }

        return ListenerResult::Continue;
    });
}

void APSSettingsPopup::apply() {
    if (!m_dirty) return;
    m_layer->updateSettings(m_settings);
    m_layer->sendFullState();
    m_dirty = false;
}

APSSettingsPopup* APSSettingsPopup::create(APSPlayLayer* layer) {
    auto ret = new APSSettingsPopup;
    if (ret->init(layer)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

}
