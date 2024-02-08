#include "signup_popup.hpp"

#include <Geode/utils/web.hpp>
#include <managers/error_queues.hpp>
#include <managers/central_server.hpp>
#include <managers/game_server.hpp>
#include <managers/account.hpp>
#include <util/net.hpp>
#include <util/format.hpp>
#include <util/crypto.hpp>

using namespace geode::prelude;

bool GlobedSignupPopup::setup() {
    this->setTitle("Authentication");
    m_closeBtn->setVisible(false);

    auto& csm = CentralServerManager::get();
    auto& am = GlobedAccountManager::get();

    auto activeServer = csm.getActive();
    if (!activeServer.has_value()) {
        return false;
    }

    auto winSize = cocos2d::CCDirector::sharedDirector()->getWinSize();

    Build<CCLabelBMFont>::create("Requesting challenge..", "bigFont.fnt")
        .pos(winSize.width / 2, winSize.height / 2 + m_size.height / 2 - 50.f)
        .scale(0.35f)
        .store(statusMessage)
        .parent(m_mainLayer);

    auto url = activeServer->url + "/challenge/new?aid=" + std::to_string(am.gdData.lock()->accountId);

    web::AsyncWebRequest()
        .userAgent(util::net::webUserAgent())
        .timeout(util::time::seconds(5))
        .post(url)
        .text()
        .then([this](std::string& response) {
            auto colonPos = response.find(':');
            auto part1 = response.substr(0, colonPos);
            auto part2 = response.substr(colonPos + 1);

            // we accept -1 as the default ()
            int levelId = util::format::parse<int>(part1).value_or(-1);

            this->onChallengeCreated(levelId, part2);
        })
        .expect([this](std::string error) {
            if (error.empty()) {
                this->onFailure("Creating challenge failed: server sent an empty response.");
            } else {
                this->onFailure("Creating challenge failed: <cy>" + util::format::formatErrorMessage(error) + "</c>");
            }
        })
        .send();

    return true;
}

void GlobedSignupPopup::onChallengeCreated(int levelId, const std::string_view chtoken) {
    // what are you cooking 😟
    // - xTymon 12.11.2023 19:03 CET

    auto hash = util::crypto::simpleHash(chtoken);
    auto authcode = util::crypto::simpleTOTP(hash);

    if (levelId == -1) {
        // skip posting the comment, server has it disabled
        this->onChallengeCompleted(authcode);
    } else {
        statusMessage->setString("Uploading results..");
        storedAuthcode = authcode;
        storedLevelId = levelId;
        GameLevelManager::sharedState()->m_commentUploadDelegate = this;
        GameLevelManager::sharedState()->uploadLevelComment(levelId, authcode + " ## globed verification test, if you see this you can manually delete the comment.", 0);
    }
}

void GlobedSignupPopup::commentUploadFinished(int) {
    GameLevelManager::sharedState()->m_commentUploadDelegate = nullptr;

    this->runAction(
        CCSequence::create(
            CCDelayTime::create(0.5f),
            CCCallFunc::create(this, callfunc_selector(GlobedSignupPopup::onDelayedChallengeCompleted)),
            nullptr
        )
    );
}

void GlobedSignupPopup::onDelayedChallengeCompleted() {
    this->onChallengeCompleted(storedAuthcode);
}

void GlobedSignupPopup::commentUploadFailed(int, CommentError e) {
    GameLevelManager::sharedState()->m_commentUploadDelegate = nullptr;
    this->onFailure(fmt::format("Comment upload failed: <cy>error {}</c>", (int)e));
}

void GlobedSignupPopup::commentDeleteFailed(int, int) {}

void GlobedSignupPopup::onChallengeCompleted(const std::string_view authcode) {
    auto& csm = CentralServerManager::get();
    auto& am = GlobedAccountManager::get();

    statusMessage->setString("Verifying..");

    auto gdData = am.gdData.lock();

    auto url = csm.getActive()->url +
        fmt::format("/challenge/verify?aid={}&aname={}&answer={}&systime={}",
                    gdData->accountId,
                    gdData->accountName,
                    authcode,
                    std::time(nullptr)
        );

    web::AsyncWebRequest()
        .userAgent(util::net::webUserAgent())
        .timeout(util::time::seconds(5))
        .post(url)
        .text()
        .then([this, &am](std::string& response) {
            // we are good! the authkey has been created and can be saved now.
            auto colonPos = response.find(':');
            auto commentId = response.substr(0, colonPos);

            log::info("Authkey created successfully, saving.");
            util::data::bytevector authkey;
            try {
                authkey = util::crypto::base64Decode(response.substr(colonPos + 1));
            } catch (const std::exception& e) {
                this->onFailure("Completing challenge failed: server sent an invalid response.");
                return;
            }

            am.storeAuthKey(util::crypto::simpleHash(authkey));
            this->onSuccess();

            // delete the comment to cleanup
            if (commentId != "none") {
                GameLevelManager::sharedState()->deleteComment(std::stoi(commentId), CommentType::Level, storedLevelId);
            }
        })
        .expect([this](std::string error) {
            if (error.empty()) {
                this->onFailure("Completing challenge failed: server sent an empty response.");
            } else {
                this->onFailure("Completing challenge failed: <cy>" + util::format::formatErrorMessage(error) + "</c>");
            }
        })
        .send();
}

void GlobedSignupPopup::onSuccess() {
    this->onClose(this);
}

void GlobedSignupPopup::onFailure(const std::string_view message) {
    ErrorQueues::get().error(message);
    this->onClose(this);
}

void GlobedSignupPopup::keyDown(cocos2d::enumKeyCodes) {
    // do nothing; the popup should be impossible to close manually
}

void GlobedSignupPopup::keyBackClicked() {
    // do nothing
}

GlobedSignupPopup* GlobedSignupPopup::create() {
    auto ret = new GlobedSignupPopup;
    if (ret->init(POPUP_WIDTH, POPUP_HEIGHT)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}