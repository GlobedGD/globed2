#include "DiscordLinkPopup.hpp"
#include "DiscordLinkAttemptPopup.hpp"
#include <core/net/NetworkManagerImpl.hpp>
#include <globed/core/Constants.hpp>
#include <globed/core/PopupManager.hpp>
#include <globed/util/gd.hpp>

#include <UIBuilder.hpp>
#include <cue/Util.hpp>

using namespace geode::prelude;

namespace globed {

const CCSize DiscordLinkPopup::POPUP_SIZE{280.f, 160.f};
static constexpr CCSize ICON_SIZE{40.f, 40.f};

bool DiscordLinkPopup::setup()
{
    this->setTitle("Link Discord Account");

    auto gam = GJAccountManager::get();
    std::string uname = gam->m_username;
    int id = gam->m_accountID;
    cue::Icons icons = getPlayerIcons();

    m_discordBtn = Build<CCSprite>::create("discord01.png"_spr)
                       .with([&](auto spr) { cue::rescaleToMatch(spr, 32.f); })
                       .intoMenuItem(+[] { utils::web::openLinkInBrowser(globed::constant<"discord">()); })
                       .pos(this->fromBottomLeft(22.f, 22.f))
                       .id("discord-btn")
                       .scaleMult(1.1f)
                       .parent(m_buttonMenu);

    Build<CCSprite>::createSpriteName("GJ_infoIcon_001.png")
        .with([&](auto spr) { cue::rescaleToMatch(spr, 20.f); })
        .intoMenuItem(+[] {
            globed::alert(
                "Discord Link",
                "In this menu you can link your <cb>Discord</c> account to your <cg>Geometry Dash</c> account.\n\n"
                "This unlocks some benefits, such as the ability to use <cj>voice chat</c> and sync your "
                "<cb>Discord</c> roles (for <cp>Supporters</c> or <cl>Moderators</c>).\n\n"
                "To start, you <cr>must</c> join our <cb>Discord</c> server, by clicking the button in the bottom "
                "left.");
        })
        .pos(this->fromBottomRight(18.f, 18.f))
        .id("info-btn")
        .scaleMult(1.1f)
        .parent(m_buttonMenu);

    m_playerCard = Build<CCNode>::create()
                       .anchorPoint(0.5f, 0.5f)
                       .contentSize(POPUP_SIZE.width * 0.85f, POPUP_SIZE.height * 0.55f)
                       .layout(RowLayout::create()->setAutoScale(false))
                       .pos(this->fromCenter(0.f, 8.f))
                       .id("player-card")
                       .parent(m_mainLayer)
                       .collect();

    m_playerIcon = Build(cue::PlayerIcon::create(icons))
                       .with([&](auto spr) { cue::rescaleToMatch(spr, ICON_SIZE); })
                       .parent(m_playerCard);

    m_dataContainer = Build<CCNode>::create()
                          .layout(ColumnLayout::create()
                                      ->setAxisReverse(true)
                                      ->setAutoScale(false)
                                      ->setCrossAxisLineAlignment(AxisAlignment::Start)
                                      ->setGap(0.f))
                          .zOrder(10)
                          .id("data-container")
                          .parent(m_playerCard)
                          .contentSize(0.f, 52.f)
                          .collect();

    m_nameLabel = Build<CCLabelBMFont>::create(uname.c_str(), "goldFont.fnt")
                      .limitLabelWidth(POPUP_SIZE.width * 0.85f, 0.6f, 0.1f)
                      .id("name-label")
                      .parent(m_dataContainer);

    // m_idLabel = Build<CCLabelBMFont>::create(fmt::format("{}", id).c_str(), "goldFont.fnt")
    //     .scale(0.5f)
    //     .color()
    //     .parent(dataContainer);

    m_statusContainer =
        Build<CCNode>::create()
            .layout(RowLayout::create()->setAutoScale(false)->setGap(3.f)->setAxisAlignment(AxisAlignment::Start))
            .contentSize(140.f, 0.f)
            .id("status-container")
            .parent(m_dataContainer);

    Build<CCLabelBMFont>::create("Status:", "bigFont.fnt").scale(0.5f).parent(m_statusContainer);

    m_statusLabel = Build<CCLabelBMFont>::create("Loading...", "bigFont.fnt").scale(0.5f).parent(m_statusContainer);

    m_statusContainer->updateLayout();
    m_dataContainer->updateLayout();
    m_playerCard->updateLayout();

    m_background = cue::attachBackground(m_playerCard);

    auto &nm = NetworkManagerImpl::get();

    m_stateListener = nm.listen<msg::DiscordLinkStateMessage>([this](const auto &msg) {
        this->onStateLoaded(msg.id, msg.username, msg.avatarUrl);
        return ListenerResult::Continue;
    });

    m_attemptListener = nm.listen<msg::DiscordLinkAttemptMessage>([this](const auto &msg) {
        this->onAttemptReceived(msg.id, msg.username, msg.avatarUrl);
        return ListenerResult::Continue;
    });

    this->requestState(0.f);
    // this->onStateLoaded(612930885230002208, "dank_meme01",
    // "https://cdn.discordapp.com/avatars/612930885230002208/57793059bc19ed4cc2c36ae1c321b423.png?size=1024");

    return true;
}

void DiscordLinkPopup::onClose(CCObject *)
{
    Popup::onClose(nullptr);
    NetworkManagerImpl::get().sendSetDiscordPairingState(false);
}

void DiscordLinkPopup::onStateLoaded(uint64_t id, const std::string &username, const std::string &avatarUrl)
{
    if (id == 0) {
        if (m_activelyWaiting)
            return;

        m_statusLabel->setString("Unlinked");
        m_statusLabel->setColor(ccColor3B{255, 64, 64});
        m_statusContainer->updateLayout();

        cue::resetNode(m_startBtn);

        m_startBtn = Build<ButtonSprite>::create("Start", "bigFont.fnt", "GJ_button_01.png", 0.8f)
                         .scale(0.95f)
                         .intoMenuItem([this](auto btn) {
                             NetworkManagerImpl::get().sendSetDiscordPairingState(true);
                             btn->removeFromParent();
                             this->addLinkingText();
                         })
                         .scaleMult(1.1f)
                         .pos(this->fromBottom(26.f))
                         .parent(m_buttonMenu);

        return;
    }

    m_activelyWaiting = false;
    this->unschedule(schedule_selector(DiscordLinkPopup::requestState));

    m_statusLabel->setString("Linked");
    m_statusLabel->setColor(ccColor3B{69, 255, 23});
    m_statusContainer->updateLayout();

    m_nameLabel->setString(username.c_str());
    m_nameLabel->limitLabelWidth(POPUP_SIZE.width * 0.85f, 0.6f, 0.1f);

    cue::resetNode(m_idLabel);
    cue::resetNode(m_playerIcon);
    cue::resetNode(m_background);
    cue::resetNode(m_waitingLabel1);
    cue::resetNode(m_waitingLabel2);
    cue::resetNode(m_avatar);

    m_dataContainer->updateLayout();

    m_avatar = Build(LazySprite::create(ICON_SIZE)).parent(m_playerCard);
    m_avatar->setAutoResize(true);
    m_avatar->loadFromUrl(convertAvatarUrl(avatarUrl));
    m_playerCard->updateLayout();

    m_background = cue::attachBackground(m_playerCard);
}

void DiscordLinkPopup::addLinkingText()
{
    cue::resetNode(m_discordBtn);

    m_waitingLabel1 = Build<CCLabelBMFont>::create("Waiting to link...", "bigFont.fnt")
                          .scale(0.5f)
                          .pos(this->fromCenter(0.f, -40.f))
                          .id("waiting-label1")
                          .parent(m_mainLayer);

    m_waitingLabel2 = Build<CCLabelBMFont>::create("Use /link command on Discord!", "bigFont.fnt")
                          .scale(0.4f)
                          .pos(this->fromBottom(24.f))
                          .id("waiting-label2")
                          .parent(m_mainLayer);
}

void DiscordLinkPopup::onAttemptReceived(uint64_t id, const std::string &username, const std::string &avatarUrl)
{
    auto popup = DiscordLinkAttemptPopup::create(id, username, avatarUrl);
    popup->setCallback([this, id](bool confirm) {
        NetworkManagerImpl::get().sendDiscordLinkConfirm(id, confirm);

        if (confirm) {
            this->startWaitingForRefresh();
        }
    });
    popup->show();
}

void DiscordLinkPopup::startWaitingForRefresh()
{
    m_activelyWaiting = true;
    this->schedule(schedule_selector(DiscordLinkPopup::requestState), 1.0f);
}

void DiscordLinkPopup::requestState(float)
{
    NetworkManagerImpl::get().sendGetDiscordLinkState();
}

} // namespace globed