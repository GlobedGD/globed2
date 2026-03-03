#pragma once

#include <globed/util/CStr.hpp>
#include <ui/BasePopup.hpp>
#include <Geode/utils/function.hpp>

namespace globed {

static constexpr auto RULES_TEXT = R"(
# Punishments

Breaking any of the rules below may result in your account being <cr>muted or banned from Globed</c>, depending on the severity of the offense.

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
We temporarily store server logs to keep <cj>Globed</c> running properly. These logs may include your <cy>account ID</c>, <cy>username</c>, <cy>platform</c>, <cy>IP address</c>, <cy>connection type</c>, the <cy>levels or rooms</c> you join, and other in-game behavior. All logs are used strictly for <cg>debugging</c> and <cg>statistical purposes</c>, and are never kept for more than 30 days.

We may also keep some anonymous data for statistics, such as which levels are played most often. This data cannot be used to identify you.

# Argon Authentication
We use [Argon](https://github.com/GlobedGD/argon) for verifying accounts. Argon sends a one-time message to a <cy>bot account</c> on the behalf of your <cg>Geometry Dash account</c>, and after verification that message is deleted. By using Globed, you agree to allow Argon to send this message.

# Other data usage
Globed may access other account data, for example your list of <cg>friends</c> or <cg>blocked users</c>. This data is <cr>never</c> logged anywhere, and is only used to provide <cj>in-game functionality</c>, such as <cy>invite or voice chat filtering</c>.
)";

using ConsentCallback = geode::Function<void(bool)>;

class ConsentPopup : public BasePopup {
public:
    static ConsentPopup* create();

    bool init();
    void setCallback(ConsentCallback&& cb);

private:
    ConsentCallback m_callback;
    CCMenuItemToggler* m_vcButton = nullptr;
    CCMenuItemToggler* m_qcButton = nullptr;
    CCMenuItemSpriteExtra* m_acceptBtn;
    bool m_rulesAccepted = false;
    bool m_privacyAccepted = false;

    void onButton(bool accept);
    CCNode* createClause(CStr description, CStr popupTitle, std::string_view content, bool* accepted);
    void updateAcceptButton();
};

}