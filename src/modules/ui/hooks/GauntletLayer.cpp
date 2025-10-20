#include <globed/config.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <globed/core/data/Messages.hpp>
#include <modules/ui/UIModule.hpp>
#include <core/net/NetworkManagerImpl.hpp>

#include <Geode/Geode.hpp>
#include <Geode/modify/GauntletLayer.hpp>
#include <UIBuilder.hpp>
#include <asp/iter.hpp>

using namespace geode::prelude;

namespace globed {

struct GLOBED_MODIFY_ATTR HookedGauntletLayer : geode::Modify<HookedGauntletLayer, GauntletLayer> {
    struct Fields {
        std::unordered_map<int, uint16_t> m_levels;
        std::unordered_map<int, CCNode*> m_wrappers;
        std::optional<MessageListener<msg::PlayerCountsMessage>> m_listener;
    };

    static void onModify(auto& self) {
        GLOBED_CLAIM_HOOKS(UIModule::get(), self,
            "GauntletLayer::init",
            "GauntletLayer::loadLevelsFinished",
        );
    }

    $override
    bool init(GauntletType type) {
        if (!GauntletLayer::init(type)) return false;

        if (m_levels) {
            this->buildUI();
        }

        return true;
    }

    $override
    void loadLevelsFinished(CCArray* p0, char const* p1, int p2) {
        GauntletLayer::loadLevelsFinished(p0, p1, p2);

        if (m_levels) {
            this->buildUI();
        }
    }

    void buildUI() {
        auto& nm = NetworkManagerImpl::get();
        if (!nm.isConnected()) return;

        auto levelsMenu = this->getChildByIDRecursive("levels-menu");
        if (levelsMenu == nullptr) {
            return;
        }

        auto levelButtons = levelsMenu->getChildrenExt<CCMenuItemSpriteExtra>();
        for (size_t i = 0; i < levelButtons.size(); i++) {
            auto wrapper = Build<CCNode>::create()
                .pos(15.f, -33.f)
                .scale(0.45f)
                .visible(false)
                .contentSize(levelButtons[i]->getScaledContentSize())
                .layout(RowLayout::create()->setGap(5.f))
                .parent(levelButtons[i])
                .id("level-playercount-wrapper"_spr)
                .collect();

            Build<CCSprite>::create("icon-person.png"_spr)
                .id("level-playercount-icon"_spr)
                .parent(wrapper);

            Build<CCLabelBMFont>::create("", "goldFont.fnt")
                .id("level-playercount-label"_spr)
                .parent(wrapper);

            wrapper->updateLayout();
            int id = CCArrayExt<GJGameLevel*>(m_levels)[i]->m_levelID;
            m_fields->m_wrappers[id] = wrapper;
        }

        m_fields->m_listener = nm.listen<msg::PlayerCountsMessage>([this](auto packet) {
            m_fields->m_levels.clear();

            for (const auto& [id, count] : packet.counts) {
                m_fields->m_levels[id.levelId()] = count;
            }

            this->refreshPlayerCounts();
            return ListenerResult::Continue;
        });

        this->schedule(schedule_selector(HookedGauntletLayer::updatePlayerCounts), 5.f);
        this->updatePlayerCounts(0.f);
    }

    void refreshPlayerCounts() {
        for (const auto& [levelId, wrapper] : m_fields->m_wrappers) {
            auto playerCount = m_fields->m_levels[levelId];

            if (playerCount == 0) {
                wrapper->setVisible(false);
            } else {
                wrapper->setVisible(true);
                auto label = static_cast<CCLabelBMFont*>(wrapper->getChildByID("level-playercount-label"_spr));
                label->setString(fmt::to_string(playerCount).c_str());
                wrapper->updateLayout();
            }
        }
    }

    void updatePlayerCounts(float) {
        auto& nm = NetworkManagerImpl::get();
        if (!nm.isConnected()) return;

        auto levelIds = asp::iter::from(CCArrayExt<GJGameLevel*>(m_levels))
            .map([](auto level) { return (uint64_t) level->m_levelID; })
            .collect();

        nm.sendRequestPlayerCounts(std::span{levelIds.data(), levelIds.size()});
    }
};

}
