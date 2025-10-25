#include "CreditsPopup.hpp"
#include <globed/core/PopupManager.hpp>
#include <globed/core/actions.hpp>
#include <core/net/NetworkManagerImpl.hpp>

#include <UIBuilder.hpp>
#include <cue/PlayerIcon.hpp>

using namespace geode::prelude;

namespace globed {

const CCSize CreditsPopup::POPUP_SIZE {380.f, 260.f };
static constexpr CCSize LIST_SIZE { 340.f, 200.f};

// TODO: credits data
static const std::vector<CreditsCategory> g_defaultData {
};

namespace {
class CreditsPlayerNode : public CCNode {
public:
    static CreditsPlayerNode* create(const globed::CreditsUser& user) {
        auto ret = new CreditsPlayerNode;
        if (ret->init(user)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }

private:
    bool init(const globed::CreditsUser& user) {
        CCNode::init();

        auto* sp = Build(cue::PlayerIcon::create(cue::Icons {
            .type = IconType::Cube,
            .id = user.cube,
            .color1 = user.color1.asIdx(),
            .color2 = user.color2.asIdx(),
            .glowColor = user.glowColor.asIdx(),
        }))
            .anchorPoint({0.5f, 0.5f})
            .zOrder(0)
            .parent(this)
            .collect();

        // shadow
        auto* shadow = Build<CCSprite>::create("shadow-thing-idk.png"_spr)
            .scale(0.55f)
            .pos(0.f, -13.f)
            .zOrder(-1)
            .parent(this)
            .collect();

        // name
        CCMenuItemSpriteExtra* nameLabel;
        auto menu = Build<CCLabelBMFont>::create(user.displayName.c_str(), "goldFont.fnt")
            .scale(0.45f)
            .limitLabelWidth(52.f, 0.45f, 0.05f)
            .intoMenuItem([accountId = user.accountId, userId = user.userId, name = user.username] {
                openUserProfile(accountId, userId, name);
            })
            .pos(0.f, 22.f)
            .store(nameLabel)
            .intoNewParent(CCMenu::create())
            .pos(0.f, 0.f)
            .parent(this)
            .collect();

        float width = sp->getScaledContentSize().width * 1.1f;
        float height = sp->getScaledContentSize().height * 1.1f + nameLabel->getScaledContentSize().height;

        this->setContentSize({width, height});

        CCPoint delta = CCPoint{width, height} / 2;
        sp->setPosition(sp->getPosition() + delta);
        shadow->setPosition(shadow->getPosition() + delta);
        menu->setPosition(menu->getPosition() + delta);

        return true;
    }

};

class CategoryCell : public CCNode {
public:
    static CategoryCell* create(const globed::CreditsCategory& cat) {
        auto ret = new CategoryCell;
        if (ret->init(cat)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }

private:
    bool init(const globed::CreditsCategory& cat) {
        CCNode::init();

        constexpr size_t PLAYERS_IN_ROW = 6;

        size_t rows = (cat.users.size() + PLAYERS_IN_ROW - 1) / PLAYERS_IN_ROW;

        auto* title = Build<CCLabelBMFont>::create(cat.name.c_str(), "bigFont.fnt")
            .scale(0.68f)
            .pos(LIST_SIZE.width / 2, 3.f)
            .parent(this)
            .collect();

        const float wrapperGap = 8.f;
        auto* playerWrapper = Build<CCNode>::create()
            .layout(ColumnLayout::create()->setAxisReverse(true)->setGap(wrapperGap))
            .pos(LIST_SIZE.width / 2.f, 0.f)
            .anchorPoint(0.5f, 0.0f)
            .contentSize(LIST_SIZE.width, 50.f)
            .id("player-wrapper"_spr)
            .parent(this)
            .collect();

        float wrapperHeight = 0.f;

        for (size_t i = 0; i < rows; i++) {
            size_t firstIdx = i * PLAYERS_IN_ROW;
            size_t lastIdx = std::min((i + 1) * PLAYERS_IN_ROW, cat.users.size());
            size_t count = lastIdx - firstIdx;

            float playerGap = 15.f;
            if (count < 4) {
                playerGap = 40.f;
            } else if (count == 4) {
                playerGap = 30.f;
            } else if (count == 5) {
                playerGap = 25.f;
            } else if (count == 6) {
                playerGap = 18.f;
            }

            auto* row = Build<CCNode>::create()
                .layout(RowLayout::create()->setGap(playerGap))
                .id("wrapper-row"_spr)
                .parent(playerWrapper)
                .contentSize(LIST_SIZE.width, 0.f)
                .collect();

            for (size_t i = firstIdx; i < lastIdx; i++) {
                row->addChild(CreditsPlayerNode::create(cat.users[i]));
            }

            row->updateLayout();

            wrapperHeight += row->getContentHeight();
            if (i != 0) {
                wrapperHeight += wrapperGap;
            }
        }

        playerWrapper->setContentSize({0.f, wrapperHeight});
        playerWrapper->updateLayout();

        this->setContentSize(CCSize{LIST_SIZE.width, playerWrapper->getScaledContentSize().height + 8.f + title->getScaledContentSize().height});

        // title at the top
        title->setPosition({title->getPositionX(), this->getContentHeight() - 10.f});

        return true;
    }
};
}

bool CreditsPopup::setup() {
    this->setTitle("Credits");

    m_list = Build(cue::ListNode::create(LIST_SIZE))
        .pos(this->fromCenter(0.f, -10.f))
        .parent(m_mainLayer);

    m_list->setAutoUpdate(false);

    auto& nm = NetworkManagerImpl::get();
    m_listener = nm.listen<msg::CreditsMessage>([this](const auto& msg) {
        if (msg.unavailable) {
            globed::alert("Error", "Credits are temporarily unavailable.");
        } else {
            this->onLoaded(msg.categories);
        }

        return ListenerResult::Stop;
    });

    if (nm.isConnected()) {
        nm.sendFetchCredits();
    } else {
        this->onLoaded(g_defaultData);
    }

    return true;
}

void CreditsPopup::onLoaded(const std::vector<CreditsCategory>& categories) {
    m_list->clear();

    for (auto& cat : categories) {
        m_list->addCell(CategoryCell::create(cat));
    }

    m_list->updateLayout();
}

}