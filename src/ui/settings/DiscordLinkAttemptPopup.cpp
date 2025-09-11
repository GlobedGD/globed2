#include "DiscordLinkAttemptPopup.hpp"

#include <UIBuilder.hpp>
#include <cue/Util.hpp>

using namespace geode::prelude;

namespace globed {

const CCSize DiscordLinkAttemptPopup::POPUP_SIZE { 300.f, 170.f};

bool DiscordLinkAttemptPopup::setup(uint64_t userId, const std::string& username, const std::string& avatarUrl) {
    this->setTitle("Link Attempt");
    m_title->setPositionY(m_title->getPositionY() + 5.f);

    m_userId = userId;

    auto card = Build<CCNode>::create()
        .anchorPoint(0.5f, 0.5f)
        .contentSize(POPUP_SIZE.width * 0.7f, POPUP_SIZE.height * 0.7f)
        .layout(RowLayout::create()->setAutoScale(false))
        .pos(this->fromCenter(0.f, 24.f))
        .parent(m_mainLayer)
        .collect();

    auto avatar = Build(LazySprite::create({ 40.f, 40.f }))
        .parent(card)
        .collect();
    avatar->setAutoResize(true);
    avatar->loadFromUrl(convertAvatarUrl(avatarUrl));

    auto dataContainer = Build<CCNode>::create()
        .layout(ColumnLayout::create()
            ->setAxisReverse(true)
            ->setAutoScale(false)
            ->setCrossAxisLineAlignment(AxisAlignment::Start)
            ->setAxisAlignment(AxisAlignment::End)
            ->setGap(0.f)
        )
        .zOrder(10)
        .parent(card)
        .contentSize(0.f, 40.f)
        .collect();

    auto nameLabel = Build<CCLabelBMFont>::create(username.c_str(), "goldFont.fnt")
        .limitLabelWidth(POPUP_SIZE.width * 0.45f, 0.7f, 0.1f)
        .parent(dataContainer);

    dataContainer->updateLayout();
    card->updateLayout();

    cue::BackgroundOptions opts{};
    opts.verticalPadding = 8.f;
    cue::attachBackground(card, opts);

    Build<CCMenu>::create()
        .layout(RowLayout::create()->setAutoScale(false)->setGap(3.f))
        .contentSize(POPUP_SIZE.width * 0.8f, 0.f)
        .pos(this->fromBottom(22.f))
        .parent(m_mainLayer)
        .child(
            Build<ButtonSprite>::create("Decline", "bigFont.fnt", "GJ_button_06.png", 0.8f)
                .scale(0.8f)
                .intoMenuItem([this] {
                    this->confirm(false);
                })
                .scaleMult(1.1f)
        )
        .child(
            Build<ButtonSprite>::create("Confirm", "bigFont.fnt", "GJ_button_01.png", 0.8f)
                .scale(0.8f)
                .intoMenuItem([this] {
                    this->confirm(true);
                })
                .scaleMult(1.1f)
        )
        .updateLayout();

    Build<CCLabelBMFont>::create("Only accept if this is your account!", "bigFont.fnt")
        .scale(0.42f)
        .color({ 255, 127, 41 })
        .pos(this->fromBottom(56.f))
        .parent(m_mainLayer);

    return true;
}

void DiscordLinkAttemptPopup::setCallback(Callback&& cb) {
    m_callback = std::move(cb);
}

void DiscordLinkAttemptPopup::confirm(bool accept) {
    if (m_callback) m_callback(accept);
    this->onClose(nullptr);
}

std::string convertAvatarUrl(const std::string& url) {
    auto copy = url;

    // we don't need much more than 256x256
    auto qpos = copy.find('?');
    if (qpos != copy.npos) {
        copy.resize(qpos);
    }
    copy += "?size=256";

    if (Loader::get()->isModLoaded("prevter.imageplus")) {
        // with image plus, we can support webp and gif images with zero issues
        return copy;
    }

    // otherwise we have to downgrade to png
    utils::string::replaceIP(copy, ".webp", ".png");
    utils::string::replaceIP(copy, ".gif", ".png");

    return copy;
}

}