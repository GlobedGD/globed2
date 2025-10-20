#include <globed/config.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <globed/core/data/Messages.hpp>
#include <globed/core/actions.hpp>
#include <globed/util/FunctionQueue.hpp>
#include <modules/ui/UIModule.hpp>
#include <core/net/NetworkManagerImpl.hpp>

#include <Geode/Geode.hpp>
#include <Geode/modify/LevelSelectLayer.hpp>
#include <UIBuilder.hpp>
#include <cue/Util.hpp>

using namespace geode::prelude;

namespace globed {

class PlayerCountLabel : public CCNode {
public:
    bool init(bool compressed) {
        if (!CCNode::init()) return false;

        this->compressed = compressed;

        if (compressed) {
            this->setLayout(RowLayout::create()->setGap(3.f));
            this->setContentWidth(100.f);

            Build<CCLabelBMFont>::create("", "bigFont.fnt")
                .parent(this)
                .store(label);

            Build<CCSprite>::create("icon-person.png"_spr)
                .parent(this)
                .store(icon);
        } else {
            Build<CCLabelBMFont>::create("", "bigFont.fnt")
                .parent(this)
                .store(label);
        }

        return true;
    }

    void updateCount(size_t players) {
        if (players == 0) {
            this->setVisible(false);
            return;
        }

        this->setVisible(true);

        if (compressed) {
            label->setString(players == -1 ? "?" : fmt::to_string(players).c_str());
            this->updateLayout();
        } else {
            label->setString(
                players == -1 ?
                    "? players" :
                    fmt::format("{} {}", players, players == 1 ? "player" : "players").c_str()
            );
        }
    }

    static PlayerCountLabel* create(bool compressed) {
        auto ret = new PlayerCountLabel;
        if (ret->init(compressed)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }

private:
    CCSprite* icon = nullptr;
    CCLabelBMFont* label = nullptr;
    bool compressed;
};


struct GLOBED_MODIFY_ATTR HookedLevelSelectLayer : geode::Modify<HookedLevelSelectLayer, LevelSelectLayer> {
    static inline const auto MAIN_LEVELS = std::to_array<uint64_t>({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22});

    struct Fields {
        std::unordered_map<int, uint16_t> m_levels;
        std::optional<MessageListener<msg::PlayerCountsMessage>> m_listener;
    };

    static void onModify(auto& self) {
        GLOBED_CLAIM_HOOKS(UIModule::get(), self,
            "LevelSelectLayer::init",
            "LevelSelectLayer::updatePageWithObject",
        );
    }

    $override
    bool init(int p0) {
        if (!LevelSelectLayer::init(p0)) return false;

        auto& nm = NetworkManagerImpl::get();
        if (!nm.isConnected()) return true;

        m_fields->m_listener = nm.listen<msg::PlayerCountsMessage>([this](auto packet) {
            m_fields->m_levels.clear();
            for (const auto& level : packet.counts) {
                m_fields->m_levels[level.first.levelId()] = level.second;
            }

            this->updatePlayerCounts();

            return ListenerResult::Continue;
        });

        this->schedule(schedule_selector(HookedLevelSelectLayer::sendRequest), 5.f);
        this->sendRequest(0.f);

        return true;
    }

    $override
    void updatePageWithObject(cocos2d::CCObject* o1, cocos2d::CCObject* o2) {
        LevelSelectLayer::updatePageWithObject(o1, o2);

        FunctionQueue::get().queue([self = Ref(this)] {
            self->updatePlayerCounts();
        });
    }

    void sendRequest(float) {
        auto& nm = NetworkManagerImpl::get();

        if (nm.isConnected()) {
            nm.sendRequestPlayerCounts(std::span{MAIN_LEVELS.data(), MAIN_LEVELS.size()});
        }
    }

    void updatePlayerCounts() {
        auto* bsl = this->getChildByType<BoomScrollLayer>(0);
        if (!bsl) return;
        auto* extlayer = bsl->getChildByType<ExtendedLayer>(0);
        if (!extlayer || extlayer->getChildrenCount() == 0) return;

        auto& fields = *m_fields.self();

        for (auto* page : CCArrayExt<LevelPage*>(extlayer->getChildren())) {
            auto buttonmenu = page->getChildByType<CCMenu>(0);
            auto button = buttonmenu->getChildByType<CCMenuItemSpriteExtra>(0);
            PlayerCountLabel* label = static_cast<PlayerCountLabel*>(button->getChildByID("player-count-label"_spr));

            int id = page->m_level->m_levelID;
            if (id < 0) {
                cue::resetNode(label);
                continue;
            }

            if (!label) {
                auto pos = (button->getContentSize() / 2) + ccp(0.f, -37.f);
                Build<PlayerCountLabel>::create(globed::setting<bool>("core.ui.compressed-player-count"))
                    .pos(pos)
                    .scale(0.4f)
                    .anchorPoint(0.5f, 0.5f)
                    .id("player-count-label"_spr)
                    .parent(button)
                    .store(label);
            }

            if (!NetworkManagerImpl::get().isConnected()) {
                label->setVisible(false);
            } else if (auto it = fields.m_levels.find(id); it != fields.m_levels.end()) {
                label->updateCount(it->second);
            } else {
                label->updateCount(0);
            }
        }
    }
};

}
