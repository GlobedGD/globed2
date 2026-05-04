#include "SettingsPopup.hpp"
#include <core/net/NetworkManagerImpl.hpp>
#include <ui/Core.hpp>

#include <AdvancedLabel.hpp>
#include <cue/LoadingCircle.hpp>
#include <cue/Util.hpp>
#include <cue/RadioLogic.hpp>

using namespace geode::prelude;

namespace globed {

bool APSSettingsPopup::init() {
    if (!Popup::init(340.f, 230.f)) return false;

    this->setTitle("Switcheroo Settings");

    auto layer = APSPlayLayer::get();
    GLOBED_ASSERT(layer);

    m_settings = layer->m_fields->m_controller.m_settings;

    m_btn = Build(ButtonSprite::create("Start", "bigFont.fnt", "GJ_button_01.png", 0.8f))
        .scale(0.85f)
        .intoMenuItem([this] {
            this->startGame();
            m_btn->removeFromParent();
        })
        .pos(this->fromBottom(24.f))
        .parent(m_buttonMenu)
        .collect();

    // left side - radio buttons for algorithm
    auto llayout = Build(ColumnContainer::create(5.f))
        .parent(m_mainLayer)
        .pos(this->fromLeft({24.f, 12.f}))
        .anchorPoint(0.f, 0.5f)
        .collect();
    llayout->layout()->setCrossAxisAlignment(CrossAxisAlignment::Start);

    std::array<std::pair<const char*, APSSelectionAlgorithm>, 3> algos = {
        std::make_pair("Random", APSSelectionAlgorithm::Random),
        std::make_pair("Fair Random", APSSelectionAlgorithm::FairRandom),
        std::make_pair("Sequential", APSSelectionAlgorithm::Sequential),
    };

    cue::RadioLogic<APSSelectionAlgorithm> radio;

    for (auto& [name, algo] : algos) {
        auto cont = Build<RowContainer>::create(5.f)
            .parent(llayout)
            .collect();

        auto btn = radio.createToggler(algo, 0.6f);
        btn->m_offButton->m_scaleMultiplier = 1.15f;
        btn->m_onButton->m_scaleMultiplier = 1.15f;
        cont->addChild(btn);

        Build<Label>::create(name, "bigFont.fnt")
            .scale(0.45f)
            .parent(cont);

        cont->updateLayout();
    }

    radio.setCallback([this](APSSelectionAlgorithm algo) {
        m_settings.m_cycleAlgo = algo;
        m_dirty = true;
    });
    radio.select(m_settings.m_cycleAlgo);

    llayout->updateLayout();
    cue::attachBackground(llayout, cue::BackgroundOptions {
        .sidePadding = 6.f,
        .verticalPadding = 6.f,
        .cornerRoundness = -0.2f
    });

    auto switchAlgoLabel = Build<Label>::create("Switch Logic", "goldFont.fnt")
        .scale(0.65f)
        .parent(m_mainLayer)
        .pos(cue::fromTopRight(llayout, {0.f, 16.f})) // it's dumb
        .collect();

    Build<CCSprite>::createSpriteName("GJ_infoIcon_001.png")
        .scale(0.5f)
        .intoMenuItem([] {
            globed::alert(
                "Switch Logic",
                "This option determines how to decide the <cy>order</c> in which players will be switched.\n\n"
                "<cj>Random</c> - selects a completely random player\n"
                "<cj>Fair Random</c> - random, but ensures that in one cycle every player will get to play once\n"
                "<cj>Sequential</c> - selects players in the same, consistent order"
            );
        })
        .scaleMult(1.15f)
        .parent(m_buttonMenu)
        .pos(cue::fromTopRight(switchAlgoLabel, {3.f, 0.f}));

    // bool whether to warn only next player
    auto warnContainer = Build<RowContainer>::create(5.f)
        .parent(m_mainLayer)
        .anchorPoint(0.f, 0.5f)
        .pos(this->fromLeft({22.f, -44.f}))
        .collect();

    auto toggler = Build(CCMenuItemExt::createTogglerWithStandardSprites(0.6f, [this](auto toggler) {
        bool on = !toggler->isOn();
        m_settings.m_showNextPlayer = on;
        m_dirty = true;
    }))
        .parent(warnContainer)
        .collect();
    toggler->m_offButton->m_scaleMultiplier = 1.15f;
    toggler->m_onButton->m_scaleMultiplier = 1.15f;
    toggler->toggle(m_settings.m_showNextPlayer);

    Build<Label>::create("Show Next Player", "bigFont.fnt")
        .scale(0.4f)
        .parent(warnContainer);

    warnContainer->updateLayout();

    // right side - sliders for interval, var, warning delay

    auto rlayout = Build(ColumnContainer::create(6.f))
        .parent(m_mainLayer)
        .anchorPoint(1.f, 0.5f)
        .pos(this->fromRight({24.f, 8.f}))
        .collect();

    std::array<std::tuple<const char*, float*, float, float>, 3> sliders = {
        std::make_tuple("Switch Interval", &m_settings.m_interval, 1.f, 30.f),
        std::make_tuple("Switch Variance", &m_settings.m_intervalVar, 0.f, 15.f),
        std::make_tuple("Warning Delay", &m_settings.m_warningDelay, 0.f, 5.f),
    };

    for (auto& [name, field, min, max] : sliders) {
        auto cont = Build<ColumnContainer>::create(2.f)
            .parent(rlayout)
            .collect();

        Build<Label>::create(name, "bigFont.fnt")
            .scale(0.35f)
            .parent(cont);

        auto sliderCont = Build<RowContainer>::create(8.f)
            .parent(cont)
            .collect();

        auto lbl = Build<Label>::create("", "bigFont.fnt")
            .scale(0.35f)
            .zOrder(2) // order after slider
            .parent(sliderCont)
            .collect();

        Build(createSlider())
            .with([&] (cue::Slider* slider) {
                slider->setCallback([this, field, lbl](cue::Slider*, double value) {
                    *field = value;
                    m_dirty = true;
                    lbl->setString(fmt::format("{:.1f}s", value));
                });
                slider->setRange(min, max);
                slider->setValue(*field);
                slider->setStep(0.1);
                slider->setContentWidth(80.f);
                lbl->setString(fmt::format("{:.1f}s", *field));
            })
            .parent(sliderCont);

        sliderCont->updateLayout();
        cont->updateLayout();
    }

    rlayout->updateLayout();
    cue::attachBackground(rlayout, cue::BackgroundOptions{
        .sidePadding = 6.f,
        .verticalPadding = 6.f
    });

    return true;
}

void APSSettingsPopup::onClose(CCObject*) {
    this->apply();
    Popup::onClose(nullptr);
}

void APSSettingsPopup::startGame() {
    auto layer = APSPlayLayer::get();
    if (!layer) {
        globed::toastError("failed to get play layer!");
        return;
    }

    m_dirty = false;

    layer->updateSettings(m_settings);
    layer->restartSwitchCycle();

    // wait until we receive back the event from the server, then close this popup
    auto circle = Build<cue::LoadingCircle>::create(true)
        .scale(0.7f)
        .parent(m_mainLayer)
        .pos(this->fromBottom(36.f));

    m_buttonMenu->setEnabled(false);

    m_listener = APSFullStateEvent::listen([this](const auto& ev) {
        this->onClose(nullptr);
        return SkipRemainingEvents{};
    }, 5);
}

void APSSettingsPopup::apply() {
    if (!m_dirty) return;
    auto layer = APSPlayLayer::get();
    if (layer) {
        layer->updateSettings(m_settings);
        layer->sendFullState();
    }
    m_dirty = false;
}

APSSettingsPopup* APSSettingsPopup::create() {
    auto ret = new APSSettingsPopup;
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

}
