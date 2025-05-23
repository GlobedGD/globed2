#include "connect_popup.hpp"

#include <managers/central_server.hpp>
#include <managers/game_server.hpp>
#include <managers/account.hpp>
#include <managers/error_queues.hpp>
#include <managers/settings.hpp>
#include <net/manager.hpp>
#include <util/format.hpp>
#include <util/gd.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;
using ConnectionState = NetworkManager::ConnectionState;

namespace {
    enum class WhatFailed {
        Message, Verify
    };

    static std::string makeErrorMessage(WhatFailed whatFailed, std::string_view why) {
        bool gdps = util::gd::isGdps();
        bool mainServer = CentralServerManager::get().isOfficialServerActive();

        std::string msg;

        switch (whatFailed) {
            case WhatFailed::Message: msg = "Message upload <cr>failed</c>."; break;
            case WhatFailed::Verify: msg = "Account verification <cr>failed</c>."; break;
            default: break;
        }

        // If they are not on a GDPS, really the only thing that makes sense here is they might need to refresh login
        if (!gdps) {
            msg += " Please try to open GD account settings and <cg>Refresh Login</c>.";

            if (whatFailed == WhatFailed::Verify) {
                msg += fmt::format("\n\nReason: <cy>{}</c>", why);
            }
        } else if (mainServer) {
            // if they are on a gdps, and using the main server, this is most definitely the issue
            if (whatFailed == WhatFailed::Verify) {
                msg += fmt::format(" Reason: <cy>{}</c>.", why);
            }

            msg += "\n\n<cp>Note:</c> You are likely using a <cj>GDPS</c>, connecting to the <cg>official Globed servers</c> is impossible when on a <cj>GDPS</c>!";
        } else {
            // if they are using a custom server, perhaps the server is misconfigured? hard to tell really
            msg += " If you are the owner of this Globed server, ensure the configuration is correct and check the logs.";

            if (whatFailed == WhatFailed::Verify) {
                msg += fmt::format("\n\nReason: <cy>{}</c>", why);
            }
        }

        return msg;
    }
}

bool ConnectionPopup::setup(GameServer gs_) {
    m_server = std::move(gs_);
    m_closeBtn->removeFromParent();
    m_closeBtn = nullptr;

    auto rlayout = util::ui::getPopupLayoutAnchored(m_size);

    Build<CCLabelBMFont>::create("", "bigFont.fnt")
        .pos(rlayout.fromCenter(0.f, -10.f))
        .scale(0.35f)
        .store(m_statusLabel)
        .parent(m_mainLayer);

    this->setTitle(fmt::format("Connecting to {}", m_server.name));

    auto& csm = CentralServerManager::get();
    if (csm.standalone()) {
        this->startStandalone();
    } else {
        this->startCentral();
    }

    this->scheduleUpdate();

    return true;
}

void ConnectionPopup::update(float dt) {
    if (m_state == State::AttemptingConnection || m_state == State::Establishing || m_state == State::RelayEstablishing) {
        auto& nm = NetworkManager::get();
        auto state = nm.getConnectionState();

        if (state == ConnectionState::Disconnected) {
            this->onAbruptDisconnect(); // abrubt disconnect
        } else if (state == ConnectionState::RelayAuthStage1 || state == ConnectionState::RelayAuthStage2) {
            this->updateState(State::RelayEstablishing); // tcp connection -> relay establishing
        } else if (state == ConnectionState::Authenticating) {
            this->updateState(State::Establishing); // tcp connection / relay establishing -> authentication
        } else if (state == ConnectionState::Established) {
            // authentication -> established
            this->updateState(State::Done);
            this->forceClose();
        }
    }
}

void ConnectionPopup::onAbruptDisconnect() {
    this->forceClose();
}

void ConnectionPopup::forceClose() {
    m_doClose = true;
    this->onClose(this);
}

void ConnectionPopup::onClose(CCObject* o) {
    if (m_doClose) {
        Popup::onClose(o);
    }
}

void ConnectionPopup::startStandalone() {
    GLOBED_RESULT_ERRC(NetworkManager::get().connectStandalone());
    this->updateState(State::AttemptingConnection);
}

void ConnectionPopup::startCentral() {
    // if we have the session token, we can just connect
    auto& am = GlobedAccountManager::get();

    auto hasToken = !am.authToken.lock()->empty();

    if (hasToken) {
        GLOBED_RESULT_ERRC(NetworkManager::get().connect(m_server));
        this->updateState(State::AttemptingConnection);
        return;
    }

    // steps here will depend on whether we need to authenticate or not

    auto& gam = GlobedAccountManager::get();
    gam.autoInitialize();

    auto& csm = CentralServerManager::get();
    bool hasAuth = csm.activeHasAuth();
    auto argonUrl = csm.activeArgonUrl();

    if (hasAuth && argonUrl) {
        // start argon auth
        m_argonState = argon::AuthProgress::RequestedChallenge;
        this->updateState(State::ArgonAuth);

        auto res = argon::setServerUrl(argonUrl.value());
        if (!res) {
            log::warn("Failed to set argon url: {}", res.unwrapErr());
        }

        argon::setCertVerification(!GlobedSettings::get().launchArgs().noSslVerification);

        res = argon::startAuth([this](Result<std::string> token) {
            auto& gam = GlobedAccountManager::get();
            if (token) {
                gam.storeArgonToken(token.unwrap());
                this->requestTokenAndConnect();
            } else {
                this->onArgonFailure(token.unwrapErr());
            }
        }, [this](auto prog) {
            this->m_argonState = prog;
            this->updateState(State::ArgonAuth);
        });

        if (!res) {
            this->onFailure("Failed to authenticate the user: <cy>{}</c>", res.unwrapErr());
        }

        return;
    } else if (gam.hasAuthKey()) {
        this->requestTokenAndConnect();
    } else {
        // well here we have to go through the outdated challenge process :(
        this->challengeStart();
    }
}

void ConnectionPopup::requestTokenAndConnect() {
    auto& am = GlobedAccountManager::get();
    auto& csm = CentralServerManager::get();

    am.requestAuthToken([self = Ref(this), server = m_server](bool done) {
        if (done) {
            GLOBED_RESULT_ERRC(NetworkManager::get().connect(server));
            self->updateState(State::AttemptingConnection);
        } else {
            self->forceClose(); // the func already shows an error message for us
        }
    }, csm.activeArgonUrl().has_value());

    this->updateState(State::CreatingSessionToken);
}

void ConnectionPopup::updateState(State state) {
    if (state == m_state) return;

    m_state = state;

    std::string message;

    using enum State;

    switch (state) {
        case None: message = "?"; break;
        case ArgonAuth: message = fmt::format("Argon: {}", argon::authProgressToString(m_argonState)); break;
        case RequestedChallenge: message = "Requested challenge.."; break;
        case UploadingChallenge: message = "Uploading results.."; break;
        case VerifyingChallenge: message = "Verifying challenge.."; break;
        case CreatingSessionToken: message = "Creating session.."; break;
        case AttemptingConnection: message = "Connecting.."; break;
        case Establishing: message = "Establishing.."; break;
        case RelayEstablishing: message = "Establishing relay.."; break;
        case Done: message = "Done!"; break;
    }

    m_statusLabel->setString(message.c_str());
    m_statusLabel->limitLabelWidth(POPUP_WIDTH * 0.85f, 0.35f, 0.1f);
}

void ConnectionPopup::onArgonFailure(const std::string& error) {
    if (error == "Stage 2 failed (generic error)") {
        log::warn("Message upload failed, checking message count");
        this->tryCheckMessageCount();
    } else {
        this->onFailure("Failed to create a token with argon: <cy>{}</c>", error);
    }
}

void ConnectionPopup::challengeStart() {
    auto request = WebRequestManager::get().challengeStart();
    createListener.bind(this, &ConnectionPopup::challengeStartCallback);
    createListener.setFilter(std::move(request));

    this->updateState(State::RequestedChallenge);
}

void ConnectionPopup::challengeStartCallback(typename WebRequestManager::Event* event) {
    if (!event || !event->getValue()) return;

    auto evalue = std::move(*event->getValue());

    if (!evalue.ok()) {
        std::string message = evalue.getError();

        log::warn("error creating challenge");
        log::warn("{}", message);

        this->onFailure("Creating challenge failed: <cy>{}</c>", message);

        return;
    }

    auto resptext = evalue.text().unwrapOrDefault();

    auto parts = util::format::split(resptext, ":");

    if (parts.size() < 3) {
        this->onFailure("Creating challenge failed: <cy>response does not consist of 3+ parts</c>");
        return;
    }

    // we accept -1 as the default
    int accountId = util::format::parse<int>(parts[0]).value_or(-1);
    std::string_view challenge = parts[1];
    std::string_view pubkey = parts[2];
    bool secureMode = false;
    if (parts.size() > 3) {
        secureMode = parts[3] == "1";
    }

    this->onChallengeCreated(accountId, challenge, pubkey, secureMode);
}

static Result<std::string> decodeAnswer(std::string_view chtoken, std::string_view pubkey) {
    GLOBED_UNWRAP_INTO(util::crypto::base64Decode(chtoken), auto decodedChallenge);
    GLOBED_UNWRAP_INTO(util::crypto::base64Decode(pubkey), auto cryptoKey);

    SecretBox box(cryptoKey);
    return box.decryptToString(decodedChallenge);
}

void ConnectionPopup::onChallengeCreated(int accountId, std::string_view chtoken, std::string_view pubkey, bool secureMode) {
    auto ans = decodeAnswer(chtoken, pubkey);
    if (!ans) {
        log::warn("failed to complete challenge: {}", ans.unwrapErr());
        this->onFailure("Failed to complete challenge: <cy>{}</c>", ans.unwrapErr());
        return;
    }

    std::string answer = ans.unwrap();

    this->storedChToken = chtoken;
    this->isSecureMode = secureMode;

    if (secureMode && !NetworkManager::get().canGetSecure()) {
        log::warn("cannot create authtoken, secure mode is disabled but server requires it");

        // mod is built in insecure mode, or secure mode initialization failed
#ifdef GLOBED_OSS_BUILD
        this->onFailure("Globed is built in the <cy>open-source mode</c>, which enables <cr>insecure mode</c>. Connecting to this server is <cr>not possible</c> while in this mode.");
#else
        this->onFailure("<cr>Secure mode initialization failed.</c> This is a bug. Please report this to the mod developer.");
#endif

        return;
    }

    if (accountId == -1) {
        // skip the account verification, server has it disabled
        this->onChallengeCompleted(answer);
    } else {
        this->updateState(State::UploadingChallenge);
        storedAuthcode = answer;
        storedAccountId = accountId;

        auto glm = GameLevelManager::get();
        glm->m_uploadMessageDelegate = this;
        glm->uploadUserMessage(storedAccountId, fmt::format("##c## {}", answer), "globed verification test, this message can be deleted");
    }
}

void ConnectionPopup::uploadMessageFinished(int) {
    GameLevelManager::get()->m_uploadMessageDelegate = nullptr;

    this->runAction(
        CCSequence::create(
            CCDelayTime::create(0.5f),
            CCCallFunc::create(this, callfunc_selector(ConnectionPopup::onDelayedChallengeCompleted)),
            nullptr
        )
    );
}

void ConnectionPopup::uploadMessageFailed(int e) {
    GameLevelManager::get()->m_uploadMessageDelegate = nullptr;
    this->tryCheckMessageCount();
}

void ConnectionPopup::onDelayedChallengeCompleted() {
    this->onChallengeCompleted(storedAuthcode);
}

void ConnectionPopup::tryCheckMessageCount() {
#ifdef GLOBED_LESS_BINDINGS
    // in the less bindings mode, just fail
    this->onFailure(
        "Message upload failed due to an unknown reason. Please try <cr>deleting</c> some <cy>sent messages</c> and <cg>Refresh Login</c> in GD account settings."
    );
#else
    // fetch user's sent messages, if they have 400 (which means they have 50 on page 7, 0-indexed) then they are at the limit and cannot send any more
    auto glm = GameLevelManager::get();
    glm->m_messageListDelegate = this;
    glm->getUserMessages(true, 7, 50);
#endif
}

void ConnectionPopup::loadMessagesFinished(cocos2d::CCArray* p0, char const* p1) {
    GameLevelManager::get()->m_messageListDelegate = nullptr;
    size_t count = p0 ? p0->count() : 0;

    if (count == 50) {
        this->forceClose();

        geode::createQuickPopup(
            "Note",
            "You are at the limit of sent messages (<cy>400</c>), and cannot send any more. To <cg>verify your account</c>, we need to send a message to a bot account. "
            "Do you want to open your messages and <cr>delete</c> some?",
            "Cancel",
            "Open",
            [](auto, bool open) {
                if (!open) return;

                MessagesProfilePage::create(true)->show();
            }
        );
    } else {
        this->onFailure("{}", makeErrorMessage(WhatFailed::Message, ""));
    }
}

void ConnectionPopup::loadMessagesFailed(char const* p0, GJErrorCode p1) {
    GameLevelManager::get()->m_messageListDelegate = nullptr;

#ifdef GLOBED_LESS_BINDINGS
    this->onFailure(fmt::format("Failed to check message count (code <cy>{}</c>). This is most likely an account issue, please try to <cg>Refresh Login</c> in GD account settings.", (int) p1));
#else
    this->forceClose();
    geode::createQuickPopup(
        "Error",
        fmt::format("Failed to check message count (code <cy>{}</c>). This is most likely an account issue, please try to <cg>Refresh Login</c> in GD account settings."
            " Open settings now?", (int) p1),
        "Cancel",
        "Open",
        [](auto, bool open) {
            if (!open) return;

            auto scene = CCScene::get();
            auto acl = AccountLayer::create();
            scene->addChild(acl, scene->getHighestChildZ() + 1);
            acl->m_accountHelpRelated = 1;
            acl->layerHidden(); // for some reason this switches to the "more" tab in account management,
                                // which is exactly what we want
        }
    );
#endif
}

// completed challenge

void ConnectionPopup::onChallengeCompleted(std::string_view authcode) {
    auto& csm = CentralServerManager::get();
    auto& am = GlobedAccountManager::get();

    this->updateState(State::VerifyingChallenge);

    auto request = WebRequestManager::get().challengeFinish(authcode, isSecureMode ? storedChToken : "");
    finishListener.bind(this, &ConnectionPopup::challengeCompleteCallback);
    finishListener.setFilter(std::move(request));
}

void ConnectionPopup::challengeCompleteCallback(typename WebRequestManager::Event* event) {
    if (!event || !event->getValue()) return;

    auto evalue = std::move(*event->getValue());

    if (!evalue.ok()) {
        if (evalue.getCode() == 401) {
            auto msg = makeErrorMessage(WhatFailed::Verify, evalue.text().unwrapOrDefault());
            this->onFailure("{}", msg);
            return;
        }

        this->onFailure("{}", evalue.getError());
        return;
    }

    auto response = evalue.text().unwrapOrDefault();

    // we are good! the authkey has been created and can be saved now.
    auto colonPos = response.find(':');
    auto messageId = response.substr(0, colonPos);

    if (colonPos == std::string::npos) {
        log::warn("invalid challenge finish response: {}", response);
        this->onFailure("Completing challenge failed: <cy>invalid response from the server (no ':' found)</c>");
        return;
    }

    auto encodedAuthkey = response.substr(colonPos + 1);

    log::info("Authkey created successfully, saving.");

    auto dres = util::crypto::base64Decode(encodedAuthkey);
    if (!dres) {
        log::warn("failed to decode authkey: {}", dres.unwrapErr());
        log::warn("authkey: {}", encodedAuthkey);
        this->onFailure("Completing challenge failed: <cy>invalid response from the server (couldn't decode base64)</c>");
        return;
    }

    auto authkey = std::move(dres.unwrap());

    auto& am = GlobedAccountManager::get();

    am.storeAuthKey(util::crypto::simpleHash(authkey));

    // complete auth
    this->requestTokenAndConnect();

    // delete the comment to cleanup
    if (messageId != "none") {
        auto message = GJUserMessage::create();
        message->m_messageID = util::format::parse<int>(messageId).value_or(-1);
        if (message->m_messageID == -1) return;

        GameLevelManager::sharedState()->deleteUserMessages(message, nullptr, true);
    }
}

ConnectionPopup* ConnectionPopup::create(GameServer gs) {
    auto ret = new ConnectionPopup;
    if (ret->initAnchored(POPUP_WIDTH, POPUP_HEIGHT, std::move(gs))) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}
