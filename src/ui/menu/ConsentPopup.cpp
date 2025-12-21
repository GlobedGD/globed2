#include "ConsentPopup.hpp"
#include <globed/core/SettingsManager.hpp>
#include <globed/core/PopupManager.hpp>
#include <UIBuilder.hpp>
#include <AdvancedLabel.hpp>
#include <cue/Util.hpp>

using namespace geode::prelude;

namespace globed {

const CCSize ConsentPopup::POPUP_SIZE {400.f, 260.f};

static constexpr auto RULES_DESC = "I agree to follow the <cg>Globed rules</c> while using the mod";
static constexpr auto ARGON_DESC = "I accept the <cy>privacy policy</c> and allow <cj>Globed</c> to send a one-time message to verify my GD account";

static constexpr auto RULES_TEXT = R"(
# Punishments

Breaking any of the rules below may result in your account being <cr>muted or banned</c>, depending on the severity of the offense.

# In-game Chat

**<cr>Please avoid:</c>**
* Micspamming (loud noises, soundboard spam, echo, music, loud clicking, etc).
* Discussions about uncomfortable or controversial topics.
* Any form of harassment or toxicity.
* Hate Speech and bigotry.

# Rooms

**Your room name <cr>should not contain</c>:**
* Advertisements (links to your socials, level names / IDs, etc).
* Slurs, hate speech, controversial or inappropriate content.

Additionally, <cr>avoid</c> mass inviting people to your room.

# Accounts

* You will be held responsible if your Geometry Dash username or profile posts contain <cr>slurs, hate speech or any form of explicit content</c>.
* You should not <cy>share your account</c> with others, as you are responsible for any actions taken on your account.

# Reporting

If you find someone breaking the rules, report them in [our Discord server](https://discord.gg/d56q5Dkdm3).
)";

static constexpr auto ARGON_TEXT = R"(
# Privacy Policy
We temporarily store server logs to keep <cj>Globed</c> running properly. These logs may include your <cy>account ID</c>, <cy>username</c>, <cy>IP address</c>, <cy>connection type</c>, the <cy>levels or rooms</c> you join, and other in-game behavior. All logs are used strictly for <cg>debugging</c> and <cg>statistical purposes</c>, and are automatically deleted whenever the server restarts.

We may also keep some anonymous data for statistics, such as which levels are played most often. This data cannot be used to identify you.

# Argon Authentication
We use [Argon](https://github.com/GlobedGD/argon) for verifying accounts. Argon sends a one-time message to a <cy>bot account</c> on the behalf of your <cg>Geometry Dash account</c>, and after verification that message is deleted. By using Globed, you agree to allow Argon to send this message.

# Other data usage
Globed may access other account data, for example your list of <cg>friends</c> or <cg>blocked users</c>. This data is <cr>never</c> logged anywhere, and is only used to provide <cj>in-game functionality</c>, such as <cy>invite or voice chat filtering</c>.
)";

bool ConsentPopup::setup() {
    this->setID("consent-popup"_spr);
    this->setTitle("Before you continue...");

    // add buttons for enabling/disabling voice chat and quick chat
    auto chatContainer = Build<CCMenu>::create()
        .layout(SimpleRowLayout::create()
            ->setGap(5.f)
            ->setMainAxisScaling(AxisScaling::Fit)
            ->setCrossAxisScaling(AxisScaling::Fit)
        )
        .anchorPoint(0.5f, 0.5f)
        .id("chat-select-container")
        .parent(m_mainLayer)
        .pos(this->fromCenter(0.f, 50.f))
        .collect();

    m_vcButton = Build(CCMenuItemExt::createTogglerWithStandardSprites(0.6f, [](auto) {}))
        .id("vc-toggle-btn")
        .parent(chatContainer);
    m_vcButton->toggle(globed::setting<bool>("core.audio.voice-chat-enabled"));

    Build<Label>::create("Voice chat", "bigFont.fnt")
        .scale(0.55f)
        .id("vc-label")
        .parent(chatContainer);

    Build(AxisGap::create(25.f))
        .parent(chatContainer);

    m_qcButton = Build(CCMenuItemExt::createTogglerWithStandardSprites(0.6f, [](auto) {}))
        .id("qc-toggle-btn")
        .parent(chatContainer);
    m_qcButton->toggle(globed::setting<bool>("core.player.quick-chat-enabled"));

    Build<Label>::create("Emotes", "bigFont.fnt")
        .scale(0.55f)
        .id("qc-label")
        .parent(chatContainer);

    chatContainer->updateLayout();
    cue::attachBackground(chatContainer, cue::BackgroundOptions {
        .sidePadding = 6.f,
        .verticalPadding = 6.f
    });

    // small text above
    Build<Label>::create("Communication with other players (both can be adjusted later in settings)", "chatFont.fnt")
        .scale(0.5f)
        .id("chat-label")
        .parent(m_mainLayer)
        .pos(cue::fromTop(chatContainer, 14.f));

    // actual consent questions (first for accepting rules, second for data collection)

    auto consentContainer = Build<CCNode>::create()
        .layout(SimpleColumnLayout::create()
            ->setGap(10.f)
            ->setMainAxisScaling(AxisScaling::Fit)
            ->setCrossAxisScaling(AxisScaling::Fit)
        )
        .anchorPoint(0.5f, 0.5f)
        .id("consent-container")
        .parent(m_mainLayer)
        .pos(this->fromCenter(0.f, -40.f))
        .collect();

    auto clause1 = Build(this->createClause(RULES_DESC, "Globed Rules", RULES_TEXT, &m_rulesAccepted))
        .parent(consentContainer)
        .collect();
    auto clause2 = Build(this->createClause(ARGON_DESC, "Privacy Policy", ARGON_TEXT, &m_privacyAccepted))
        .parent(consentContainer)
        .collect();

    consentContainer->updateLayout();
    // cue::attachBackground(consentContainer, cue::BackgroundOptions {
    //     .sidePadding = 6.f,
    //     .verticalPadding = 6.f
    // });

    // small text
    Build<Label>::create("You must accept these to play online:", "chatFont.fnt")
        .scale(0.55f)
        .id("consent-label")
        .parent(m_mainLayer)
        .pos(cue::fromTop(consentContainer, 14.f));

    // buttons at the bottom
    auto buttonMenu = Build<CCMenu>::create()
        .layout(SimpleRowLayout::create()
            ->setGap(3.f)
            ->setCrossAxisScaling(AxisScaling::Fit)
            ->setMainAxisScaling(AxisScaling::Fit)
        )
        .parent(m_mainLayer)
        .anchorPoint(0.5f, 0.f)
        .pos(this->fromBottom(8.f))
        .collect();

    Build<ButtonSprite>::create("Decline", "bigFont.fnt", "GJ_button_06.png", 0.7f)
        .scale(0.9f)
        .intoMenuItem([this] {
            this->onButton(false);
        })
        .scaleMult(1.15f)
        .parent(buttonMenu);

    m_acceptBtn = Build<ButtonSprite>::create("Accept", "bigFont.fnt", "GJ_button_01.png", 0.7f)
        .scale(0.9f)
        .cascadeColor(true)
        .intoMenuItem([this] {
            this->onButton(true);
        })
        .cascadeColor(true)
        .scaleMult(1.15f)
        .parent(buttonMenu);

    buttonMenu->updateLayout();

    this->updateAcceptButton();

    return true;
}

CCNode* ConsentPopup::createClause(CStr description, CStr popupTitle, std::string_view content, bool* accepted) {
    float width = POPUP_SIZE.width * 0.9f;
    float textWidth = width - 64.f;

    auto container = Build<CCMenu>::create()
        .contentSize(width, 0.f)
        .layout(SimpleRowLayout::create()
            ->setGap(5.f)
            ->setMainAxisScaling(AxisScaling::Fit)
            ->setCrossAxisScaling(AxisScaling::Fit)
        )
        .collect();

    auto descLabel = Build(Label::createWrapped("", "chatFont.fnt", textWidth))
        .scale(0.65f)
        .id("clause-desc-label")
        .parent(container)
        .collect();
    colorizeLabel(descLabel, description);
    descLabel->updateChars();

    Build<CCSprite>::createSpriteName("GJ_infoIcon_001.png")
        .scale(1.f)
        .intoMenuItem([content, popupTitle] {
            MDPopup::create(std::string(popupTitle), std::string(content), "Ok")->show();
        })
        .scaleMult(1.15f)
        .parent(container);

    Build(CCMenuItemExt::createTogglerWithStandardSprites(0.8f, [this, accepted](auto btn) {
        *accepted = !btn->isOn();
        this->updateAcceptButton();
    }))
        .parent(container);

    container->updateLayout();

    cue::attachBackground(container, cue::BackgroundOptions {
        .sidePadding = 8.f,
        .verticalPadding = 4.f
    });

    return container;
}

void ConsentPopup::setCallback(ConsentCallback&& cb) {
    m_callback = std::move(cb);
}

void ConsentPopup::updateAcceptButton() {
    bool enable = m_privacyAccepted && m_rulesAccepted;
    m_acceptBtn->setEnabled(enable);
    m_acceptBtn->setColor(enable ? ccColor3B{255, 255, 255} : ccColor3B{127, 127, 127});
}

void ConsentPopup::onButton(bool accept) {
    if (accept) {
        // save vc and qc settings
        globed::setting<bool>("core.audio.voice-chat-enabled") = m_vcButton->isOn();
        globed::setting<bool>("core.player.quick-chat-enabled") = m_qcButton->isOn();
    }

    bool accepting = accept && m_rulesAccepted && m_privacyAccepted;

    if (m_callback) {
        m_callback(accepting);
    }

    Popup::onClose(this);
}

}
