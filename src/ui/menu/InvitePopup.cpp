#include "InvitePopup.hpp"
#include <ui/misc/PlayerListCell.hpp>
#include <core/net/NetworkManagerImpl.hpp>

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

const CCSize InvitePopup::POPUP_SIZE { 420.f, 280.f };
static constexpr CCSize LIST_SIZE { 336.f, 220.f };
static constexpr float CELL_HEIGHT = 27.f;
static constexpr CCSize CELL_SIZE{LIST_SIZE.width, CELL_HEIGHT};

namespace {

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

}

bool InvitePopup::setup() {
    this->setTitle("Invite Players");

    m_list = Build(cue::ListNode::create(LIST_SIZE))
        .pos(this->fromCenter(0.f, -10.f))
        .parent(m_mainLayer);
    m_list->setAutoUpdate(false);

    auto& nm = NetworkManagerImpl::get();

    m_listener = nm.listen<msg::GlobalPlayersMessage>([this](const auto& msg) {
        this->onLoaded(msg.players);
        return ListenerResult::Stop;
    });

    nm.sendRequestGlobalPlayerList("");

    // TODO: refreshing, filtering, etc..

    return true;
}

void InvitePopup::onLoaded(const std::vector<MinimalRoomPlayer>& players) {
    log::debug("Got {} players", players.size());

    m_list->clear();

    for (auto& player : players) {
        m_list->addCell(PlayerCell::create(
            player.accountData.accountId,
            player.accountData.userId,
            player.accountData.username,
            player.toIcons(),
            std::nullopt,
            CELL_SIZE
        ));
    }

    m_list->updateLayout();
}

}