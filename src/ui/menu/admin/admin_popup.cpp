#include "admin_popup.hpp"

#include "send_notice_popup.hpp"
#include "user_popup.hpp"
#include <data/packets/client/admin.hpp>
#include <data/packets/server/admin.hpp>
#include <managers/account.hpp>
#include <managers/admin.hpp>
#include <managers/error_queues.hpp>
#include <net/manager.hpp>
#include <ui/general/ask_input_popup.hpp>
#include <ui/general/intermediary_loading_popup.hpp>
#include <util/misc.hpp>
#include <util/format.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

class DummyUserLoadNode : public CCNode, public LevelManagerDelegate {
public:
    std::function<void(CCArray*)> onSuccess;
    std::function<void(int)> onError;

    static DummyUserLoadNode* create(std::function<void(CCArray*)> onSuccess, std::function<void(int)> onError) {
        auto ret = new DummyUserLoadNode;
        if (ret->init(onSuccess, onError)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }

    bool init(std::function<void(CCArray*)> onSuccess, std::function<void(int)> onError) {
        if (!CCNode::init()) return false;
        this->onSuccess = onSuccess;
        this->onError = onError;
        return true;
    }

    void loadLevelsFinished(cocos2d::CCArray* p0, char const* p1) { loadLevelsFinished(p0, p1, -1); }
    void loadLevelsFailed(char const* p0) { loadLevelsFailed(p0, -1); }
    void setupPageInfo(gd::string p0, char const* p1) {}
    void loadLevelsFinished(cocos2d::CCArray* p0, char const* p1, int p2) {
        onSuccess(p0);
    }
    void loadLevelsFailed(char const* p0, int p1) {
        onError(p1);
    }
};

bool AdminPopup::setup() {
    this->setTitle("Globed Admin Panel");

    auto sizes = util::ui::getPopupLayout(m_size);

    auto& nm = NetworkManager::get();
    auto& am = AdminManager::get();

    bool authorized = am.authorized();
    if (!authorized) return false;

    nm.addListener<AdminUserDataPacket>(this, [](auto packet) {
        AdminUserPopup::create(packet->userEntry, packet->accountData)->show();
    });

    nm.addListener<AdminErrorPacket>(this, [this](auto packet) {
        // incredibly scary code

        if (packet->message.find("failed to find the user by name") != std::string::npos) {
            // try to search the user in gd
            auto username = this->userInput->getString();
            IntermediaryLoadingPopup::create([this, username = std::move(username)](auto popup) {
                auto* dummyNode = DummyUserLoadNode::create([popup](CCArray* users) {
                    if (!users || users->count() == 0) {
                        ErrorQueues::get().warn("Unable to find the user");
                    } else {
                        auto* user = static_cast<GJUserScore*>(users->objectAtIndex(0));
                        auto& nm = NetworkManager::get();
                        nm.send(AdminGetUserStatePacket::create(std::to_string(user->m_accountID)));
                    }
                    popup->onClose(popup);
                }, [popup](auto error) {
                    ErrorQueues::get().warn(fmt::format("Failed to fetch user: {}", error == -1 ? "not found" : std::to_string(error)));
                    popup->onClose(popup);
                });
                dummyNode->setID("dummy-userload-node");
                popup->addChild(dummyNode);

                auto* glm = GameLevelManager::sharedState();
                glm->m_levelManagerDelegate = dummyNode;
                glm->getUsers(GJSearchObject::create(SearchType::Users, username));
            }, [](auto popup) {
                popup->removeChildByID("dummy-userload-node");
                auto* glm = GameLevelManager::sharedState();
                glm->m_levelManagerDelegate = nullptr;
            })->show();
        } else {
            ErrorQueues::get().warn(packet->message);
        }
    }, true);

    auto* topRightCorner = Build<CCMenu>::create()
        .layout(RowLayout::create()->setGap(5.f)->setAxisReverse(true))
        .pos(sizes.topRight - CCPoint{10.f, 30.f})
        .contentSize(65.f, 0.f)
        .anchorPoint(1.f, 0.5f)
        .parent(m_mainLayer)
        .collect();

    // logout button
    Build<CCSprite>::createSpriteName("icon-logout.png"_spr)
        .scale(1.25f)
        .intoMenuItem([this](auto) {
            GlobedAccountManager::get().clearAdminPassword();
            AdminManager::get().deauthorize();
            this->onClose(this);
        })
        .parent(topRightCorner);

    // kick all button
    Build<CCSprite>::createSpriteName("icon-kick-all.png"_spr)
        .scale(1.0f)
        .intoMenuItem([this](auto) {
            geode::createQuickPopup("Confirm action", "Are you sure you want to kick <cy>all players on the server</c>?", "Cancel", "Confirm", [this](auto, bool btn2) {
                if (!btn2) return;

                AskInputPopup::create("Kick everyone", [this](auto reason) {
                    NetworkManager::get().send(AdminDisconnectPacket::create("@everyone", reason));
                }, 100, "Kick reason", util::misc::STRING_PRINTABLE_INPUT, 1.f)->show();
            });
        })
        .parent(topRightCorner);

    topRightCorner->updateLayout();

    // send notice menu
    auto* sendNoticeWrapper = Build<CCMenu>::create()
        .layout(RowLayout::create()->setGap(5.f))
        .pos(sizes.center.width, sizes.bottom + 40.f)
        .contentSize(POPUP_WIDTH - 50.f, 50.f)
        .parent(m_mainLayer)
        .collect();

    Build<InputNode>::create(POPUP_WIDTH * 0.75f, "message", "chatFont.fnt", std::string(util::misc::STRING_PRINTABLE_INPUT), 160)
        .parent(sendNoticeWrapper)
        .store(messageInput);

    Build<ButtonSprite>::create("Send", "bigFont.fnt", "GJ_button_01.png", 0.5f)
        .intoMenuItem([this](auto) {
            auto message = this->messageInput->getString();
            if (!message.empty()) {
                AdminSendNoticePopup::create(message)->show();
            }
        })
        .parent(sendNoticeWrapper);

    sendNoticeWrapper->updateLayout();

    // find user menu

    auto* findUserWrapper = Build<CCMenu>::create()
        .layout(RowLayout::create()->setGap(5.f))
        .pos(sizes.center.width, sizes.center.height)
        .parent(m_mainLayer)
        .collect();

    Build<InputNode>::create(POPUP_WIDTH * 0.75f, "user", "chatFont.fnt", std::string(util::misc::STRING_ALPHANUMERIC), 16)
        .parent(findUserWrapper)
        .store(userInput);

    Build<ButtonSprite>::create("Find", "bigFont.fnt", "GJ_button_01.png", 0.5f)
        .intoMenuItem([this](auto) {
            auto user = this->userInput->getString();
            if (!user.empty()) {
                auto& nm = NetworkManager::get();
                nm.send(AdminGetUserStatePacket::create(user));
            }
        })
        .parent(findUserWrapper);

    findUserWrapper->updateLayout();

    return true;
}

AdminPopup* AdminPopup::create() {
    auto ret = new AdminPopup;
    if (ret->init(POPUP_WIDTH, POPUP_HEIGHT)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}