#include <globed/core/PopupManager.hpp>
#include <globed/util/CCData.hpp>
#include <core/hooks/GJBaseGameLayer.hpp>
#include <ui/BasePopup.hpp>
#include <ui/Emojis.hpp>

#include <AdvancedLabel.hpp>
#include <UIBuilder.hpp>
#include <asp/time/SystemTime.hpp>
#include <asp/data/Cow.hpp>

using namespace geode::prelude;
using namespace asp::time;
using namespace asp::data;

namespace globed {

static const std::string FIELDS_ID = "popupref-fields"_spr;
static constexpr int MANAGED_ALERT_TAG = 93583452;

struct PopupRef::Data {
    std::optional<asp::time::SystemTime> shownAt;
    size_t shownAtFrame;
    asp::time::Duration blockClosingFor;
    bool persist = false;
};

PopupRef::PopupRef(FLAlertLayer* layer) : inner(layer) {}
PopupRef::PopupRef() : inner(nullptr) {}

PopupRef::operator FLAlertLayer*() const {
    return inner;
}

FLAlertLayer* PopupRef::operator->() const {
    return inner;
}

FLAlertLayer* PopupRef::getInner() const {
    return inner;
}

void PopupRef::setPersistent(bool state) {
    this->getFields().persist = state;
}

void PopupRef::blockClosingFor(const asp::time::Duration& dur) {
    this->getFields().blockClosingFor = dur;
}

void PopupRef::blockClosingFor(int durMillis) {
    return this->blockClosingFor(Duration::fromMillis(durMillis));
}

void PopupRef::showInstant() {
    this->doShow(false);
}

void PopupRef::showQueue() {
    PopupManager::get().queuePopup(*this);
}

bool PopupRef::isShown() {
    return this->inner->getParent();
}

bool PopupRef::shouldPreventClosing() {
    auto& fields = this->getFields();

    if (!fields.shownAt || fields.blockClosingFor.isZero()) {
        return false;
    }

    auto expiry = fields.shownAt.value() + fields.blockClosingFor;
    return expiry.isFuture();
}

void PopupRef::doShow(bool reshowing) {
    if (!reshowing && inner->getParent()) {
        return;
    }

    inner->setTag(MANAGED_ALERT_TAG);
    inner->show();

    auto& fields = this->getFields();
    fields.shownAt = SystemTime::now();
    fields.shownAtFrame = PopupManager::get().m_frameCounter;
}

PopupRef::Data& PopupRef::getFields() {
    using DataT = CCData<PopupRef::Data>;

    auto obj = typeinfo_cast<DataT*>(inner->getUserObject(FIELDS_ID));

    if (!obj) {
        obj = DataT::create(Data {});
        inner->setUserObject(FIELDS_ID, obj);
    }

    return obj->data();
}

bool PopupRef::hasFields() const {
    using DataT = CCData<PopupRef::Data>;

    return typeinfo_cast<DataT*>(inner->getUserObject(FIELDS_ID));
}

// CustomFLAlert

class CustomFLAlert : public BasePopup<CustomFLAlert, CStr, CStr, CStr, cocos2d::CCNode*> {
public:
    static CustomFLAlert* create(CStr title, std::string_view content, CStr btn1, CStr btn2, float width);

private:
    bool setup(CStr title, CStr btn1, CStr btn2, CCNode* content) override;
};

bool CustomFLAlert::setup(CStr title, CStr btn1, CStr btn2, CCNode* content) {
    m_closeBtn->setVisible(false);

    content->setPosition(m_size.width / 2.f, m_size.height / 2.f + 5.f);
    m_mainLayer->addChild(content);

    this->setTitle(title.get(), "goldFont.fnt", 0.9f, 25.f);

    // confirm / cancel buttons
    auto bottomMenu = Build<CCMenu>::create()
        .layout(RowLayout::create()->setGap(3.f)->setAutoScale(false))
        .contentSize(m_size.width, 60.f)
        .pos(this->fromBottom(27.f))
        .parent(m_mainLayer)
        .collect();

    Build<ButtonSprite>::create(btn1, "goldFont.fnt", "GJ_button_01.png", 1.0f)
        .scale(0.85f)
        .intoMenuItem([this] {
            this->onClose(nullptr);
        })
        .scaleMult(1.15f)
        .parent(bottomMenu);

    if (btn2) {
        Build<ButtonSprite>::create(btn2, "goldFont.fnt", "GJ_button_01.png", 1.0f)
            .scale(0.85f)
            .intoMenuItem([this] {
                this->onClose(nullptr);
            })
            .scaleMult(1.15f)
            .parent(bottomMenu);
    }

    bottomMenu->updateLayout();

    return true;

}
CustomFLAlert* CustomFLAlert::create(
    CStr title,
    std::string_view content,
    CStr btn1,
    CStr btn2,
    float width
) {
    std::string cont = std::string(content);
    globed::translateEmojiString(cont);

    auto label = Label::createWrapped("", "chatFont.fnt", BMFontAlignment::Center, width);
    label->enableEmojis("emojis.png"_spr, getEmojiMap());
    label->setString(cont);

    CCSize size = label->getScaledContentSize() + CCSize{20.f, 120.f};

    auto ret = new CustomFLAlert();
    if (ret->initAnchored(std::max<float>(size.width, 350.f), size.height, title, btn1, btn2, label, "square01_001.png", {0.f, 0.f, 94.f, 94.f})) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

// PopupManager


PopupManager::PopupManager() {}

PopupRef PopupManager::alert(
    CStr title,
    const std::string& content,
    CStr btn1,
    CStr btn2,
    float width
) {
    return this->quickPopup(title, content, btn1, btn2, {}, width);
}

PopupRef PopupManager::quickPopup(
    CStr title,
    const std::string& content,
    CStr btn1,
    CStr btn2,
    std::function<void (FLAlertLayer*, bool)> callback,
    float width
) {
    FLAlertLayer* alert;

    if (!callback) {
        if (globed::containsEmoji(content)) {
            alert = CustomFLAlert::create(title, content, btn1, btn2.getOrNull(), width);
        } else {
            alert = FLAlertLayer::create(nullptr, title, content, btn1, btn2.getOrNull(), width);
        }
    } else {
        alert = geode::createQuickPopup(title, content, btn1, btn2.getOrNull(), width, [callback = std::move(callback)](auto alert, bool btn2) {
            callback(alert, btn2);
        }, false);
    }

    return this->manage(alert);
}

PopupRef PopupManager::manage(FLAlertLayer* alert) {
    PopupRef ref(alert);

    return ref;
}

bool PopupManager::isManaged(FLAlertLayer* alert) {
    return this->manage(alert).hasFields();
}

void PopupManager::queuePopup(const PopupRef& popup) {
    m_queuedPopups.push(popup);
}

void PopupManager::update(float dt) {
    m_frameCounter++;

    auto scene = globed::cachedSingleton<CCDirector>()->m_pRunningScene;
    if (!scene) return;

    if (scene != m_prevScene) {
        this->changedScene(scene);
    }

    // check if we are eligible to show a queued popup

    // if none, there is nothing to show
    if (m_queuedPopups.empty()) {
        return;
    }

    // check if we are transitioning 🏳️‍⚧️
    if (m_isTransitioning) {
        return;
    }

    // if we are playing and are unpaused, don't show any popups
    if (auto pl = GlobedGJBGL::get()) {
        if (!pl->isPaused()) {
            return;
        }
    }

    // if we are in loading layer then wait until menulayer
    auto children = scene->getChildren();
    if (children) {
        auto child = children->firstObject();
        if (typeinfo_cast<LoadingLayer*>(child)) {
            return;
        }
    }

    // show the popup
    auto popup = std::move(m_queuedPopups.front());
    popup.doShow();
    m_queuedPopups.pop();
}

void PopupManager::changedScene(CCScene* newScene) {
    m_prevScene = newScene;

    auto trans = typeinfo_cast<CCTransitionScene*>(newScene);

    // if we are transitioning, let's save all our popups and bring them to the other scene if needed
    if (trans) {
        m_isTransitioning = true;

        std::array<Ref<FLAlertLayer>, 16> alerts;
        size_t alertCount = 0;

        auto oldScene = trans->m_pOutScene;

        for (auto child : CCArrayExt<CCNode>(oldScene->getChildren())) {
            if (child->m_nTag == MANAGED_ALERT_TAG) {
                if (auto alert = typeinfo_cast<FLAlertLayer*>(child)) {
                    alerts[alertCount++] = alert;

                    if (alertCount == alerts.size()) break;
                }
            }
        }

        for (size_t i = 0; i < alertCount; i++) {
            auto alert = alerts[i];
            alert->removeFromParentAndCleanup(false);
        }

        m_savedAlerts = std::move(alerts);
        m_savedAlertCount = alertCount;
    } else if (m_isTransitioning) {
        m_isTransitioning = false;

        // if we were transitioning but now switched to the actual scene, restore the popups
        for (size_t i = 0; i < m_savedAlertCount; i++) {
            auto alert = this->manage(m_savedAlerts[i]);
            if (!alert || !alert.hasFields()) continue;

            // only show alert if persistent it enabled or if it has been visible for a small amount of time
            auto& fields = alert.getFields();
            auto sinceShown = fields.shownAt.value_or(SystemTime::UNIX_EPOCH).elapsed();
            if (sinceShown < Duration::fromSecsF32(1.5f) || fields.persist) {
                alert.doShow(true);
                // remove from the list
                m_savedAlerts[i] = nullptr;
            }
        }


        for (size_t i = 0; i < m_savedAlertCount; i++) {
            // cleanup those that we won't be adding back
            if (m_savedAlerts[i]) {
                m_savedAlerts[i]->cleanup();
            }

            // reset all to null
            m_savedAlerts[i] = nullptr;
        }

        m_savedAlertCount = 0;
    }
}

void toast(geode::NotificationIcon icon, float duration, const std::string& message) {
    Notification::create(message, icon, duration)->show();
}

void toast(cocos2d::CCSprite* icon, float duration, const std::string& message) {
    Notification::create(message, icon, duration)->show();
}

}
