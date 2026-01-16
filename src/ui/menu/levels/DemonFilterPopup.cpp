#include "DemonFilterPopup.hpp"

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

const CCSize DemonFilterPopup::POPUP_SIZE{300.f, 114.f};

bool DemonFilterPopup::setup(const LevelListLayer::Filters &filters)
{
    using enum Difficulty;

    m_demonTypes = filters.demonDifficulty;

    this->setTitle("Demon Filters");

    // Difficulty selector

    constexpr static ccColor3B activeColor = {255, 255, 255};
    constexpr static ccColor3B inactiveColor = {125, 125, 125};

    CCNode *difficultyRoot;
    Build<CCNode>::create()
        .id("difficulty-root")
        .contentSize({m_size.width * 0.95f, 54.f})
        .parent(m_mainLayer)
        .anchorPoint(0.5f, 0.f)
        .pos(this->fromBottom(10.f))
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

    auto *difficultyMenu = Build<CCMenu>::create()
                               .layout(RowLayout::create()->setAutoScale(false)->setGap(15.f))
                               .pos(difficultyRoot->getScaledContentSize() / 2.f + CCPoint{0.f, 2.f})
                               .parent(difficultyRoot)
                               .collect();

    bool isDemonSelected = filters.difficulty.contains(Difficulty::HardDemon);
    m_isFilteringGeneral = (isDemonSelected && filters.demonDifficulty.empty());

    constexpr float btnScale = 0.8f;

    // Demon button - different from the rest
    Build<CCSprite>::createSpriteName("difficulty_06_btn_001.png")
        .scale(btnScale)
        .intoMenuItem([this](CCMenuItemSpriteExtra *btn) {
            bool willEnable = btn->getColor().r != activeColor.r;

            if (willEnable) {
                btn->setColor(activeColor);
                m_demonTypes.clear();
                m_isFilteringGeneral = true;

                for (const auto &[key, btn] : m_demonButtons) {
                    btn->setColor(inactiveColor);
                }
            } else {
                btn->setColor(inactiveColor);
                m_isFilteringGeneral = false;
            }
        })
        .color(m_isFilteringGeneral ? activeColor : inactiveColor)
        .scaleMult(1.15f)
        .parent(difficultyMenu)
        .store(m_generalDemonButton);

    for (auto [name, type] : std::initializer_list<std::tuple<const char *, Difficulty>>{
             {"difficulty_07_btn2_001.png", EasyDemon},
             {"difficulty_08_btn2_001.png", MediumDemon},
             {"difficulty_06_btn2_001.png", HardDemon},
             {"difficulty_09_btn2_001.png", InsaneDemon},
             {"difficulty_10_btn2_001.png", ExtremeDemon},
         }) {
        auto btn = Build<CCSprite>::createSpriteName(name)
                       .scale(btnScale)
                       .intoMenuItem([this, type](CCMenuItemSpriteExtra *btn) {
                           bool willEnable = btn->getColor().r != activeColor.r;

                           if (willEnable) {
                               btn->setColor(activeColor);
                               m_demonTypes.insert(type);
                               m_generalDemonButton->setColor(inactiveColor);
                               m_isFilteringGeneral = false;
                           } else {
                               btn->setColor(inactiveColor);
                               m_demonTypes.erase(type);
                           }
                       })
                       .color((isDemonSelected && m_demonTypes.contains(type)) ? activeColor : inactiveColor)
                       .scaleMult(1.15f)
                       .parent(difficultyMenu)
                       .collect();

        m_demonButtons.emplace(std::make_pair(type, btn));
    }

    difficultyMenu->updateLayout();

    return true;
}

void DemonFilterPopup::setCallback(Callback &&cb)
{
    m_callback = std::move(cb);
}

void DemonFilterPopup::onClose(CCObject *o)
{
    BasePopup::onClose(o);

    if (m_callback) {
        m_callback(m_isFilteringGeneral || !m_demonTypes.empty(), std::move(m_demonTypes));
    }
}

} // namespace globed