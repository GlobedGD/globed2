#include "level_select_layer.hpp"

#include <hooks/gjgamelevel.hpp>
#include <data/packets/client/general.hpp>
#include <data/packets/server/general.hpp>
#include <net/manager.hpp>
#include <managers/settings.hpp>

using namespace geode::prelude;

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

            Build<CCSprite>::createSpriteName("icon-person.png"_spr)
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
            label->setString(players == -1 ? "?" : std::to_string(players).c_str());
            this->updateLayout();
        } else {
            label->setString(
                players == -1 ?
                    "? players" :
                    fmt::format("{} {}", players, players == 1 ? "player" : "players").c_str()
            );
        }

        // this->setContentSize(label->getScaledContentSize());
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

bool HookedLevelSelectLayer::init(int p0) {
    if (!LevelSelectLayer::init(p0)) return false;

    auto& nm = NetworkManager::get();
    if (!nm.established()) return true;

    nm.addListener<LevelPlayerCountPacket>(this, [this](auto packet) {
        auto currentLayer = getChildOfType<LevelSelectLayer>(CCScene::get(), 0);
        if (currentLayer && this != currentLayer) return;

        m_fields->levels.clear();
        for (const auto& level : packet->levels) {
            m_fields->levels[level.first] = level.second;
        }

        this->updatePlayerCounts();
    });

    this->schedule(schedule_selector(HookedLevelSelectLayer::sendRequest), 5.f);
    this->sendRequest(0.f);

    return true;
}
void HookedLevelSelectLayer::sendRequest(float) {
    auto& nm = NetworkManager::get();
    if (nm.established()) {
        nm.send(RequestPlayerCountPacket::create(std::move(std::vector<LevelId>(MAIN_LEVELS.begin(), MAIN_LEVELS.end()))));
    }
}

void HookedLevelSelectLayer::updatePlayerCounts() {
    auto* bsl = getChildOfType<BoomScrollLayer>(this, 0);
    if (!bsl) return;
    auto* extlayer = getChildOfType<ExtendedLayer>(bsl, 0);
    if (!extlayer || extlayer->getChildrenCount() == 0) return;

    for (auto* page : CCArrayExt<LevelPage*>(extlayer->getChildren())) {
        auto buttonmenu = getChildOfType<CCMenu>(page, 0);
        auto button = getChildOfType<CCMenuItemSpriteExtra>(buttonmenu, 0);
        PlayerCountLabel* label = static_cast<PlayerCountLabel*>(button->getChildByID("player-count-label"_spr));

        LevelId levelId = HookedGJGameLevel::getLevelIDFrom(page->m_level);
        if (levelId < 0) {
            if (label) label->setVisible(false);
            continue;
        }

        if (!label) {
            auto pos = (button->getContentSize() / 2) + ccp(0.f, -37.f);
            Build<PlayerCountLabel>::create(GlobedSettings::get().globed.compressedPlayerCount)
                .pos(pos)
                .scale(0.4f)
                .anchorPoint(0.5f, 0.5f)
                .id("player-count-label"_spr)
                .parent(button)
                .store(label);
        }

        if (!NetworkManager::get().established()) {
            label->setVisible(false);
        } else if (m_fields->levels.contains(levelId)) {
            auto players = m_fields->levels[levelId];
            label->updateCount(players);
        } else {
            label->updateCount(-1);
        }
    }
}

void HookedLevelSelectLayer::updatePageWithObject(cocos2d::CCObject* o1, cocos2d::CCObject* o2) {
    LevelSelectLayer::updatePageWithObject(o1, o2);

    this->retain();
    Loader::get()->queueInMainThread([this] {
        this->updatePlayerCounts();
        this->release();
    });
}