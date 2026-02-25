#include "InvitePopup.hpp"
#include <ui/misc/PlayerListCell.hpp>
#include <ui/misc/InputPopup.hpp>
#include <core/net/NetworkManagerImpl.hpp>

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

namespace { namespace $anon {

static constexpr CCSize LIST_SIZE { 336.f, 220.f };
static constexpr float CELL_HEIGHT = 27.f;
static constexpr CCSize CELL_SIZE{LIST_SIZE.width, CELL_HEIGHT};

class PlayerCell : public PlayerListCell {
public:
    static PlayerListCell* create(
        int accountId,
        int userId,
        const std::string& username,
        const cue::Icons& icons,
        const std::optional<SpecialUserData>& sud,
        cocos2d::CCSize cellSize
    ) {
        auto ret = new PlayerCell;
        if (ret->init(accountId, userId, username, icons, sud, cellSize)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }

protected:
    bool customSetup() override {
        CCSize btnSize { 22.f, 22.f };

        Build<CCSprite>::create("icon-invite.png"_spr)
            .with([&](auto spr) { cue::rescaleToMatch(spr, btnSize); })
            .intoMenuItem([this] {
                NetworkManagerImpl::get().sendInvitePlayer(m_accountId);
            })
            .zOrder(9)
            .scaleMult(1.1f)
            .parent(m_rightMenu);

        m_rightMenu->updateLayout();

        this->initGradients(Context::Invites);

        return true;
    }
};

} }

bool InvitePopup::init() {
    if (!BasePopup::init(420.f, 280.f)) return false;

    this->setTitle("Invite Players");

    m_list = Build(cue::ListNode::create($anon::LIST_SIZE))
        .pos(this->fromCenter(0.f, -10.f))
        .parent(m_mainLayer);
    m_list->setAutoUpdate(false);

    auto colLayout = ColumnLayout::create()
        ->setAutoScale(false)
        ->setGap(3.f)
        ->setAxisReverse(true)
        ->setAxisAlignment(AxisAlignment::End);
    colLayout->ignoreInvisibleChildren(true);

    // add some buttons

    m_rightSideMenu = Build<CCMenu>::create()
        .id("right-side-menu")
        .layout(colLayout)
        .contentSize(m_size.width * 0.08f, m_size.height - 12.f)
        .pos(m_size - CCSize{7.f, 8.f})
        .anchorPoint(1.f, 1.f)
        .parent(m_mainLayer);

    float btnSize = 30.f;

    Build<CCSprite>::create("refresh01.png"_spr)
        .with([&](auto spr) { cue::rescaleToMatch(spr, btnSize); })
        .intoMenuItem([this] {
            this->refresh();
        })
        .zOrder(9)
        .scaleMult(1.1f)
        .id("refresh-btn")
        .parent(m_rightSideMenu);

    m_searchBtn = Build<CCSprite>::create("search01.png"_spr)
        .with([&](auto spr) { cue::rescaleToMatch(spr, btnSize); })
        .intoMenuItem([this] {
            this->promptFilter();
        })
        .zOrder(10)
        .scaleMult(1.1f)
        .id("search-btn")
        .parent(m_rightSideMenu);

    m_clearSearchBtn = Build<CCSprite>::create("search02.png"_spr)
        .with([&](auto spr) { cue::rescaleToMatch(spr, btnSize); })
        .intoMenuItem([this] {
            this->setFilter("");
        })
        .zOrder(11)
        .scaleMult(1.1f)
        .id("clear-search-btn")
        .parent(m_rightSideMenu);

    m_rightSideMenu->updateLayout();

    auto& nm = NetworkManagerImpl::get();

    m_listener = nm.listen<msg::GlobalPlayersMessage>([this](const auto& msg) {
        this->onLoaded(msg.players);
        return ListenerResult::Stop;
    });

    this->refresh();

    return true;
}

void InvitePopup::promptFilter() {
    auto popup = InputPopup::create("bigFont.fnt");
    popup->setTitle("Search Player");
    popup->setPlaceholder("Username");
    popup->setMaxCharCount(16);
    popup->setCommonFilter(CommonFilter::Name);
    popup->setWidth(240.f);
    popup->setCallback([this](auto outcome) {
        if (outcome.cancelled) return;

        this->setFilter(outcome.text);
    });
    popup->show();
}

void InvitePopup::refresh() {
    NetworkManagerImpl::get().sendRequestGlobalPlayerList(m_filter);

    m_clearSearchBtn->setVisible(!m_filter.empty());
    m_searchBtn->setVisible(m_filter.empty());
    m_rightSideMenu->updateLayout();
}

void InvitePopup::setFilter(std::string_view filter) {
    m_filter = filter;
    this->refresh();
}

void InvitePopup::onLoaded(const std::vector<MinimalRoomPlayer>& players) {
    log::debug("Got {} players", players.size());

    m_list->clear();

    for (auto& player : players) {
        m_list->addCell($anon::PlayerCell::create(
            player.accountData.accountId,
            player.accountData.userId,
            player.accountData.username,
            player.toIcons(),
            std::nullopt,
            $anon::CELL_SIZE
        ));
    }

    m_list->updateLayout();
}

InvitePopup* InvitePopup::create() {
    auto ret = new InvitePopup();
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

}