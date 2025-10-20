#include <globed/config.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <globed/core/data/Messages.hpp>
#include <modules/ui/UIModule.hpp>
#include <core/net/NetworkManagerImpl.hpp>

#include <Geode/Geode.hpp>
#include <Geode/modify/LevelAreaInnerLayer.hpp>
#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

struct GLOBED_MODIFY_ATTR HookedLevelAreaInnerLayer : geode::Modify<HookedLevelAreaInnerLayer, LevelAreaInnerLayer> {
    static inline const auto TOWER_LEVELS = std::to_array<uint64_t>({5001, 5002, 5003, 5004});

    struct Fields {
        std::unordered_map<int, uint16_t> m_levels;
        std::unordered_map<int, Ref<cocos2d::CCNode>> m_doorNodes;
        std::optional<MessageListener<msg::PlayerCountsMessage>> m_listener;
    };

    static void onModify(auto& self) {
        GLOBED_CLAIM_HOOKS(UIModule::get(), self,
            "LevelAreaInnerLayer::init",
        );
    }

    $override
    bool init(bool p0) {
        if (!LevelAreaInnerLayer::init(p0)) return false;

        auto& nm = NetworkManagerImpl::get();
        if (!nm.isConnected()) return true;

        auto* mainMenu = this->getChildByIDRecursive("main-menu");
        if (!mainMenu) return true;

        for (auto id : TOWER_LEVELS) {
            auto* door = mainMenu->getChildByID(fmt::format("level-{}-button", id));
            if (!door) continue;

            auto* doorSprite = door->getChildByType<CCSprite>(0);

            const float scale = 0.45f;

            auto* wrapper = Build<CCNode>::create()
                .pos(0.f, 12.f)
                .visible(false)
                .scale(scale)
                .contentSize(doorSprite->getScaledContentSize().width / scale, 0.f)
                .layout(RowLayout::create()->setGap(5.f))
                .parent(door)
                .id("door-playercount-wrapper"_spr)
                .collect();

            Build<CCLabelBMFont>::create("", "goldFont.fnt")
                .id("door-playercount-label"_spr)
                .parent(wrapper);

            Build<CCSprite>::create("icon-person.png"_spr)
                .id("door-playercount-icon"_spr)
                .parent(wrapper);

            wrapper->updateLayout();

            m_fields->m_doorNodes[id] = wrapper;
        }

        m_fields->m_listener = nm.listen<msg::PlayerCountsMessage>([this](auto packet) {
            m_fields->m_levels.clear();

            for (const auto& [id, count] : packet.counts) {
                m_fields->m_levels[id.levelId()] = count;
            }

            this->updatePlayerCounts();
            return ListenerResult::Continue;
        });

        this->schedule(schedule_selector(HookedLevelAreaInnerLayer::sendRequest), 5.f);
        this->sendRequest(0.f);

        return true;
    }

    void sendRequest(float) {
        auto& nm = NetworkManagerImpl::get();

        if (nm.isConnected()) {
            nm.sendRequestPlayerCounts(std::span{TOWER_LEVELS.data(), TOWER_LEVELS.size()});
        }
    }

    void updatePlayerCounts() {
        auto& fields = *m_fields.self();

        for (auto id : TOWER_LEVELS) {
            if (!fields.m_doorNodes.contains(id)) continue;

            auto doorNode = fields.m_doorNodes[id];
            auto it = fields.m_levels.find(id);
            auto count = it != fields.m_levels.end() ? it->second : 0;

            if (count == 0) {
                doorNode->setVisible(false);
                continue;
            }

            doorNode->setVisible(true);
            auto label = static_cast<CCLabelBMFont*>(doorNode->getChildByID("door-playercount-label"_spr));
            if (!label) continue;

            label->setString(fmt::to_string(count).c_str());
            doorNode->updateLayout();
        }
    }
};

}
