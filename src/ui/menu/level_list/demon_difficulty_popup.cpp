#include "demon_difficulty_popup.hpp"

#include <util/ui.hpp>

using namespace geode::prelude;

bool GlobedDemonFilterPopup::setup(const GlobedLevelListLayer::Filters& filters, Callback&& cb) {
    using enum util::gd::Difficulty;

    this->callback = std::move(cb);
    this->demonTypes = filters.demonDifficulty;

    this->setTitle("Demon Filters");

    auto rlayout = util::ui::getPopupLayoutAnchored(m_size);

    // Difficulty selector

    constexpr static ccColor3B activeColor = {255, 255, 255};
    constexpr static ccColor3B inactiveColor = {125, 125, 125};

    CCNode* difficultyRoot;
    Build<CCNode>::create()
        .id("difficulty-root")
        .contentSize({rlayout.popupSize.width * 0.95f, 54.f})
        .parent(m_mainLayer)
        .anchorPoint(0.5f, 0.f)
        .pos(rlayout.fromBottom(10.f))
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
        .layout(RowLayout::create()->setAutoScale(false)->setGap(15.f))
        .pos(difficultyRoot->getScaledContentSize() / 2.f + CCPoint{0.f, 2.f})
        .parent(difficultyRoot)
        .collect();

    bool isDemonSelected = filters.difficulty.contains(util::gd::Difficulty::HardDemon);
    this->isFilteringGeneral = (isDemonSelected && filters.demonDifficulty.empty());

    constexpr float btnScale = 0.8f;

    // Demon button - different from the rest
    Build<CCSprite>::createSpriteName("difficulty_06_btn_001.png")
        .scale(btnScale)
        .intoMenuItem([this](CCMenuItemSpriteExtra* btn) {
            bool willEnable = btn->getColor().r != activeColor.r;

            if (willEnable) {
                btn->setColor(activeColor);
                this->demonTypes.clear();
                this->isFilteringGeneral = true;

                for (const auto& [key, btn] : demonButtons) {
                    btn->setColor(inactiveColor);
                }
            } else {
                btn->setColor(inactiveColor);
                this->isFilteringGeneral = false;
            }
        })
        .color(isFilteringGeneral ? activeColor : inactiveColor)
        .scaleMult(1.15f)
        .parent(difficultyMenu)
        .store(generalDemonButton);

    for (auto [name, type] : std::initializer_list<std::tuple<const char*, util::gd::Difficulty>>{
        {"difficulty_07_btn2_001.png", EasyDemon},
        {"difficulty_08_btn2_001.png", MediumDemon},
        {"difficulty_06_btn2_001.png", HardDemon},
        {"difficulty_09_btn2_001.png", InsaneDemon},
        {"difficulty_10_btn2_001.png", ExtremeDemon},
    }) {
        auto btn = Build<CCSprite>::createSpriteName(name)
            .scale(btnScale)
            .intoMenuItem([this, type](CCMenuItemSpriteExtra* btn) {
                bool willEnable = btn->getColor().r != activeColor.r;

                if (willEnable) {
                    btn->setColor(activeColor);
                    this->demonTypes.insert(type);
                    this->generalDemonButton->setColor(inactiveColor);
                    this->isFilteringGeneral = false;
                } else {
                    btn->setColor(inactiveColor);
                    this->demonTypes.erase(type);
                }
            })
            .color((isDemonSelected && this->demonTypes.contains(type)) ? activeColor : inactiveColor)
            .scaleMult(1.15f)
            .parent(difficultyMenu)
            .collect();

        demonButtons.emplace(std::make_pair(type, btn));
    }

    difficultyMenu->updateLayout();

    return true;
}

void GlobedDemonFilterPopup::onClose(CCObject* o) {
    Popup::onClose(o);
    callback(isFilteringGeneral || !demonTypes.empty(), std::move(demonTypes));
}

GlobedDemonFilterPopup* GlobedDemonFilterPopup::create(const GlobedLevelListLayer::Filters& filters, Callback&& cb) {
    auto ret = new GlobedDemonFilterPopup;
    if (ret->initAnchored(300.f, 114.f, filters, std::forward<Callback>(cb))) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}