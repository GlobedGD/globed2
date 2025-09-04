#include "LevelFiltersPopup.hpp"
#include "DemonFilterPopup.hpp"

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

const CCSize LevelFiltersPopup::POPUP_SIZE { 400.f, 280.f };

bool LevelFiltersPopup::setup(LevelListLayer* layer) {
    using Difficulty = globed::Difficulty;
    using enum Difficulty;

    m_layer = layer;
    m_filters = layer->m_filters;

    this->setTitle("Level Filters");

    auto rootLayout = Build<CCNode>::create()
        .id("root")
        .layout(
            ColumnLayout::create()
                    ->setAutoScale(false)
                    ->setAxisAlignment(AxisAlignment::Start)
                    ->setAxisReverse(true)
        )
        .parent(m_mainLayer)
        .contentSize({m_size.width * 0.95f, m_size.height * 0.8f})
        .anchorPoint(0.5f, 0.5f)
        .pos(this->fromCenter(0.f, -12.f))
        .collect();

    // Toggles

    auto* togglesMenu = Build<CCMenu>::create()
        .layout(RowLayout::create()
                    ->setAutoScale(false)
                    ->setGrowCrossAxis(true)
                    ->setAxisAlignment(AxisAlignment::Start))
        .parent(m_mainLayer)
        .pos(this->fromTop(36.f) + CCPoint{16.f, 0.f})
        .anchorPoint(0.5f, 1.f)
        .contentSize({m_size.width * 0.95f, 160.f})
        .collect();

    auto addToggle = [&](const char* label, bool initial, auto callback){
        CCMenuItemToggler* toggler;

        Build<CCMenu>::create()
            .pos(0.f, 0.f)
            .id(fmt::format("check-{}", label))
            .layout(
                RowLayout::create()
                    ->setAutoScale(false)
                    ->setGap(7.f)
                    ->setAxisReverse(true)
                    ->setAxisAlignment(AxisAlignment::Start)
            )
            .parent(togglesMenu)
            .child(Build<CCLabelBMFont>::create(label, "bigFont.fnt").scale(0.3f).collect())
            .intoNewChild(CCMenuItemExt::createTogglerWithStandardSprites(0.65f, [callback = std::move(callback)](auto btn) {
                callback(!btn->isToggled());
            }))
            .store(toggler)
            .with([&](auto btn) { btn->toggle(initial); })
            .intoParent()
            .with([&](auto node) {
                node->setContentWidth(122.f);
                node->updateLayout();
            })
            ;

        return toggler;
    };

    m_btnUncompleted = addToggle("Uncompleted", m_filters.completed && !*m_filters.completed, [this](bool state) {
        if (state) {
            m_filters.completed = false;
            m_btnCompleted->toggle(false);
        } else {
            m_filters.completed = std::nullopt;
        }
    });

    m_btnCompleted = addToggle("Completed", m_filters.completed && *m_filters.completed, [this](bool state) {
        if (state) {
            m_filters.completed = true;
            m_btnUncompleted->toggle(false);
        } else {
            m_filters.completed = std::nullopt;
        }
    });

    addToggle("Original", m_filters.original, [this](bool state) {
        m_filters.original = state;
    });

    addToggle("Coins", m_filters.coins && *m_filters.coins, [this](bool state) {
        m_filters.coins = state ? std::optional{true} : std::nullopt;
    });

    addToggle("2-Player", m_filters.twoPlayer, [this](bool state) {
        m_filters.twoPlayer = state;
    });

    // addToggle("Song", m_filters.twoPlayer, [](bool state) {});

    using RateTier = Filters::RateTier;
    using enum RateTier;

    auto modifyRateTier = [this](RateTier which, bool state) {
        if (state) {
            if (!m_filters.rateTier) {
                m_filters.rateTier.emplace(std::set<RateTier>{});
            }

            m_filters.rateTier->insert(which);
        } else {
            if (m_filters.rateTier) {
                m_filters.rateTier->erase(which);
                if (m_filters.rateTier->empty()) {
                    m_filters.rateTier = std::nullopt;
                }
            }
        }
    };

    addToggle("No Star", m_filters.rateTier && m_filters.rateTier->contains(Unrated), [modifyRateTier](bool state) {
        modifyRateTier(Unrated, state);
    });

    addToggle("Featured", m_filters.rateTier && m_filters.rateTier->contains(Feature), [modifyRateTier](bool state) {
        modifyRateTier(Feature, state);
    });

    addToggle("Epic", m_filters.rateTier && m_filters.rateTier->contains(Epic), [modifyRateTier](bool state) {
        modifyRateTier(Epic, state);
    });

    addToggle("Legendary", m_filters.rateTier && m_filters.rateTier->contains(Legendary), [modifyRateTier](bool state) {
        modifyRateTier(Legendary, state);
    });

    addToggle("Mythic", m_filters.rateTier && m_filters.rateTier->contains(Mythic), [modifyRateTier](bool state) {
        modifyRateTier(Mythic, state);
    });

    togglesMenu->updateLayout();

    // Difficulty selector

    constexpr static ccColor3B activeColor = {255, 255, 255};
    constexpr static ccColor3B inactiveColor = {125, 125, 125};

    CCNode* difficultyRoot;
    Build<CCNode>::create()
        .id("difficulty-root")
        .contentSize({m_size.width * 0.95f, 48.f})
        .parent(rootLayout)
        .store(difficultyRoot)
        .intoNewChild(CCScale9Sprite::create("square02_001.png", {0.f, 0.f, 80.f, 80.f}))
        .opacity(60)
        .id("bg")
        .with([&](auto spr) {
            auto cs = spr->getParent()->getContentSize();
            cs.height *= 2.f;
            cs.width *= 1.5f;
            spr->setContentSize(cs);
            spr->setAnchorPoint({0.f, 0.f});
        })
        .scaleY(0.5f)
        .scaleX(1.f / 1.5f);

    auto* difficultyMenu = Build<CCMenu>::create()
        .layout(RowLayout::create()->setAutoScale(false)->setGap(20.f))
        .pos(difficultyRoot->getScaledContentSize() / 2.f)
        .parent(difficultyRoot)
        .collect();

    for (auto [name, diff] : std::initializer_list<std::tuple<const char*, Difficulty>>{
        {"difficulty_00_btn_001.png", NA},
        {"difficulty_01_btn_001.png", Easy},
        {"difficulty_02_btn_001.png", Normal},
        {"difficulty_03_btn_001.png", Hard},
        {"difficulty_04_btn_001.png", Harder},
        {"difficulty_05_btn_001.png", Insane},
        {"difficulty_06_btn_001.png", HardDemon},
        {"difficulty_auto_btn_001.png", Auto},
    }) {
        auto* btn = Build<CCSprite>::createSpriteName(name)
            .scale(0.8f)
            .intoMenuItem([this, diff](CCMenuItemSpriteExtra* btn) {
                if (diff == HardDemon) {
                    auto popup = DemonFilterPopup::create(m_filters);
                    popup->setCallback([this](bool active, auto&& demonDiffs) {
                        if (!active) {
                            m_filters.demonDifficulty.clear();
                            m_filters.difficulty.erase(HardDemon);
                            this->diffButtons[HardDemon]->setColor(inactiveColor);
                        } else {
                            m_filters.demonDifficulty = std::move(demonDiffs);
                            m_filters.difficulty.insert(HardDemon);
                            this->diffButtons[HardDemon]->setColor(activeColor);
                        }
                    });
                    popup->show();

                    btn->setColor(activeColor);

                    return;
                }

                bool willEnable = btn->getColor().r != activeColor.r;

                if (willEnable) {
                    btn->setColor(activeColor);
                    m_filters.difficulty.insert(diff);
                } else {
                    btn->setColor(inactiveColor);
                    m_filters.difficulty.erase(diff);
                }
            })
            .color(m_filters.difficulty.contains(diff) ? activeColor : inactiveColor)
            .scaleMult(1.15f)
            .parent(difficultyMenu)
            .collect();

        diffButtons.emplace(std::make_pair(diff, btn));
    }

    difficultyMenu->updateLayout();

    // Length selector

    CCNode* lengthRoot;
    Build<CCNode>::create()
        .id("length-root")
        .contentSize({m_size.width * 0.95f, 36.f})
        .parent(rootLayout)
        .store(lengthRoot)
        .intoNewChild(CCScale9Sprite::create("square02_001.png", {0.f, 0.f, 80.f, 80.f}))
        .opacity(60)
        .id("bg")
        .with([&](auto spr) {
            auto cs = spr->getParent()->getContentSize();
            cs.height *= 2.f;
            cs.width *= 1.5f;
            spr->setContentSize(cs);
            spr->setAnchorPoint({0.f, 0.f});
        })
        .scaleY(0.5f)
        .scaleX(1.f / 1.5f);

    auto* lengthMenu = Build<CCMenu>::create()
        .layout(RowLayout::create()->setAutoScale(false)->setGap(10.f))
        .pos(lengthRoot->getScaledContentSize() / 2.f)
        .parent(lengthRoot)
        .collect();

    Build<CCSprite>::createSpriteName("GJ_timeIcon_001.png")
        .parent(lengthMenu);

    for (auto [name, len] : std::initializer_list<std::tuple<const char*, int>>{
        {"Tiny", 0},
        {"Short", 1},
        {"Medium", 2},
        {"Long", 3},
        {"XL", 4},
        {"Plat.", 5}
    }) {

        Build<CCLabelBMFont>::create(name, "bigFont.fnt")
            .scale(0.5f)
            .intoMenuItem([this, len](CCMenuItemSpriteExtra* btn) {
                bool willEnable = btn->getColor().r != activeColor.r;

                if (willEnable) {
                    btn->setColor(activeColor);
                    m_filters.length.insert(len);
                } else {
                    btn->setColor(inactiveColor);
                    m_filters.length.erase(len);
                }
            })
            .color(m_filters.length.contains(len) ? activeColor : inactiveColor)
            .scaleMult(1.15f)
            .parent(lengthMenu);
    }

    Build<CCSprite>::createSpriteName("GJ_sStarsIcon_001.png")
        .scale(1.2f)
        .intoMenuItem([this](CCMenuItemSpriteExtra* btn) {
            bool willEnable = btn->getColor().r != activeColor.r;

            if (willEnable) {
                btn->setColor(activeColor);
                m_filters.rated = true;
            } else {
                btn->setColor(inactiveColor);
                m_filters.rated = false;
            }
        })
        .color(m_filters.rated ? activeColor : inactiveColor)
        .scaleMult(1.15f)
        .parent(lengthMenu);

    lengthMenu->updateLayout();

    rootLayout->updateLayout();

    return true;
}

void LevelFiltersPopup::setCallback(Callback&& cb) {
    m_callback = std::move(cb);
}

void LevelFiltersPopup::onClose(CCObject*) {
    m_callback(m_filters);
    BasePopup::onClose(nullptr);
}

}
