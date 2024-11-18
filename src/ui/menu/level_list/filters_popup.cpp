#include "filters_popup.hpp"

#include <util/ui.hpp>
#include "demon_difficulty_popup.hpp"

using namespace geode::prelude;

bool GlobedLevelFilterPopup::setup(const GlobedLevelListLayer::Filters& filters, GlobedLevelListLayer* ll, Callback&& cb) {
    using Difficulty = util::gd::Difficulty;
    using enum Difficulty;

    this->callback = std::move(cb);
    this->levelListLayer = ll;
    this->filters = filters;

    this->setTitle("Level Filters");

    auto rlayout = util::ui::getPopupLayoutAnchored(m_size);

    auto rootLayout = Build<CCNode>::create()
        .id("root")
        .layout(
            ColumnLayout::create()
                    ->setAutoScale(false)
                    ->setAxisAlignment(AxisAlignment::Start)
                    ->setAxisReverse(true)
        )
        .parent(m_mainLayer)
        .contentSize({rlayout.popupSize.width * 0.95f, rlayout.popupSize.height * 0.8f})
        .anchorPoint(0.5f, 0.5f)
        .pos(rlayout.fromCenter({0.f, -12.f}))
        .collect();

    // Toggles

    auto* togglesMenu = Build<CCMenu>::create()
        .layout(RowLayout::create()
                    ->setAutoScale(false)
                    ->setGrowCrossAxis(true)
                    ->setAxisAlignment(AxisAlignment::Start))
        .parent(m_mainLayer)
        .pos(rlayout.fromTop(36.f) + CCPoint{16.f, 0.f})
        .anchorPoint(0.5f, 1.f)
        .contentSize({rlayout.popupSize.width * 0.95f, 160.f})
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

    btnUncompleted = addToggle("Uncompleted", filters.completed && !*filters.completed, [this](bool state) {
        if (state) {
            this->filters.completed = false;
            this->btnCompleted->toggle(false);
        } else {
            this->filters.completed = std::nullopt;
        }
    });

    btnCompleted = addToggle("Completed", filters.completed && *filters.completed, [this](bool state) {
        if (state) {
            this->filters.completed = true;
            this->btnUncompleted->toggle(false);
        } else {
            this->filters.completed = std::nullopt;
        }
    });

    addToggle("Original", filters.original, [this](bool state) {
        this->filters.original = state;
    });

    addToggle("Coins", filters.coins && *filters.coins, [this](bool state) {
        this->filters.coins = state ? std::optional{true} : std::nullopt;
    });

    addToggle("2-Player", filters.twoPlayer, [this](bool state) {
        this->filters.twoPlayer = state;
    });

    // addToggle("Song", filters.twoPlayer, [](bool state) {});

    using enum GlobedLevelListLayer::Filters::RateTier;
    using RateTier = GlobedLevelListLayer::Filters::RateTier;

    auto modifyRateTier = [this](RateTier which, bool state) {
        if (state) {
            if (!this->filters.rateTier) {
                this->filters.rateTier.emplace(std::set<RateTier>{});
            }

            this->filters.rateTier->insert(which);
        } else {
            if (this->filters.rateTier) {
                this->filters.rateTier->erase(which);
                if (this->filters.rateTier->empty()) {
                    this->filters.rateTier = std::nullopt;
                }
            }
        }
    };

    addToggle("No Star", filters.rateTier && filters.rateTier->contains(Unrated), [modifyRateTier](bool state) {
        modifyRateTier(Unrated, state);
    });

    addToggle("Featured", filters.rateTier && filters.rateTier->contains(Feature), [modifyRateTier](bool state) {
        modifyRateTier(Feature, state);
    });

    addToggle("Epic", filters.rateTier && filters.rateTier->contains(Epic), [modifyRateTier](bool state) {
        modifyRateTier(Epic, state);
    });

    addToggle("Legendary", filters.rateTier && filters.rateTier->contains(Legendary), [modifyRateTier](bool state) {
        modifyRateTier(Legendary, state);
    });

    addToggle("Mythic", filters.rateTier && filters.rateTier->contains(Mythic), [modifyRateTier](bool state) {
        modifyRateTier(Mythic, state);
    });

    togglesMenu->updateLayout();

    // Difficulty selector

    constexpr static ccColor3B activeColor = {255, 255, 255};
    constexpr static ccColor3B inactiveColor = {125, 125, 125};

    CCNode* difficultyRoot;
    Build<CCNode>::create()
        .id("difficulty-root")
        .contentSize({rlayout.popupSize.width * 0.95f, 48.f})
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

    for (auto [name, diff] : std::initializer_list<std::tuple<const char*, util::gd::Difficulty>>{
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
                    GlobedDemonFilterPopup::create(
                        this->filters,
                        [this](bool active, auto&& demonDiffs) {
                            log::debug("Active: {}", active);
                            for (auto diff : demonDiffs) {
                                log::debug("diff = {}", (int)diff);
                            }
                            if (!active) {
                                this->filters.demonDifficulty.clear();
                                this->filters.difficulty.erase(HardDemon);
                                this->diffButtons[HardDemon]->setColor(inactiveColor);
                            } else {
                                this->filters.demonDifficulty = std::move(demonDiffs);
                                this->filters.difficulty.insert(HardDemon);
                                this->diffButtons[HardDemon]->setColor(activeColor);
                            }
                        }
                    )->show();

                    btn->setColor(activeColor);

                    return;
                }

                bool willEnable = btn->getColor().r != activeColor.r;

                if (willEnable) {
                    btn->setColor(activeColor);
                    this->filters.difficulty.insert(diff);
                } else {
                    btn->setColor(inactiveColor);
                    this->filters.difficulty.erase(diff);
                }
            })
            .color(this->filters.difficulty.contains(diff) ? activeColor : inactiveColor)
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
        .contentSize({rlayout.popupSize.width * 0.95f, 36.f})
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
                    this->filters.length.insert(len);
                } else {
                    btn->setColor(inactiveColor);
                    this->filters.length.erase(len);
                }
            })
            .color(this->filters.length.contains(len) ? activeColor : inactiveColor)
            .scaleMult(1.15f)
            .parent(lengthMenu);
    }

    Build<CCSprite>::createSpriteName("GJ_sStarsIcon_001.png")
        .scale(1.2f)
        .intoMenuItem([this](CCMenuItemSpriteExtra* btn) {
            bool willEnable = btn->getColor().r != activeColor.r;

            if (willEnable) {
                btn->setColor(activeColor);
                this->filters.rated = true;
            } else {
                btn->setColor(inactiveColor);
                this->filters.rated = false;
            }
        })
        .color(this->filters.rated ? activeColor : inactiveColor)
        .scaleMult(1.15f)
        .parent(lengthMenu);

    lengthMenu->updateLayout();

    rootLayout->updateLayout();

    return true;
}

void GlobedLevelFilterPopup::onClose(CCObject* o) {
    Popup::onClose(o);
    this->callback(this->filters);
}

GlobedLevelFilterPopup* GlobedLevelFilterPopup::create(const GlobedLevelListLayer::Filters& filters, GlobedLevelListLayer* ll, Callback&& cb) {
    auto winSize = CCDirector::get()->getWinSize();

    auto ret = new GlobedLevelFilterPopup;
    if (ret->initAnchored(400.f, 260.f, filters, ll, std::forward<Callback>(cb))) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}
