#include "popup.hpp"

#include <hooks/gjbasegamelayer.hpp>
#include <util/cocos.hpp>
#include <Geode/modify/FLAlertLayer.hpp>

#include <asp/data/Cow.hpp>
#include <asp/time/SystemTime.hpp>

using namespace asp::data;
using namespace asp::time;
using namespace geode::prelude;
using PopupRef = PopupManager::PopupRef;


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
    using DataT = util::cocos::CCData<PopupRef::Data>;

    auto obj = typeinfo_cast<DataT*>(inner->getUserObject(FIELDS_ID));

    if (!obj) {
        obj = DataT::create(Data {});
        inner->setUserObject(FIELDS_ID, obj);
    }

    return obj->data();
}

bool PopupRef::hasFields() {
    using DataT = util::cocos::CCData<PopupRef::Data>;

    return typeinfo_cast<DataT*>(inner->getUserObject(FIELDS_ID));
}

PopupManager::PopupManager() {}

PopupRef PopupManager::alert(
    const char* title,
    const std::string& content,
    const char* btn1,
    const char* btn2,
    float width
) {
    return this->quickPopup(title, content, btn1, btn2, {}, width);
}

PopupRef PopupManager::alert(
    const std::string& title,
    const std::string& content,
    const char* btn1,
    const char* btn2,
    float width
) {
    return alert(title.c_str(), content, std::move(btn1), std::move(btn2), std::move(width));
}

PopupRef PopupManager::quickPopup(
    const std::string& title,
    const std::string& content,
    const char* btn1,
    const char* btn2,
    std::function<void (FLAlertLayer*, bool)> callback,
    float width
) {
    return this->quickPopup(title.c_str(), content, btn1, btn2, std::move(callback), width);
}

PopupRef PopupManager::quickPopup(
    const char* title,
    const std::string& content,
    const char* btn1,
    const char* btn2,
    std::function<void (FLAlertLayer*, bool)> callback,
    float width
) {
    if (btn2 && strlen(btn2) == 0) {
        btn2 = nullptr;
    }

    FLAlertLayer* alert;
    if (!callback) {
        alert = FLAlertLayer::create(nullptr, title, content, btn1, btn2, width);
    } else {
        alert = geode::createQuickPopup(title, content, btn1, btn2, [callback = std::move(callback)](auto alert, bool btn2) {
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

        std::array<Ref<FLAlertLayer>, 8> alerts;
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

struct GLOBED_DLL FLAlertLayerHook : geode::Modify<FLAlertLayerHook, FLAlertLayer> {
    void keyBackClicked() {
        auto& pm = PopupManager::get();
        if (!pm.isManaged(this)) {
            return FLAlertLayer::keyBackClicked();
        }

        auto pref = pm.manage(this);
        auto& fields = pref.getFields();

        if (!fields.shownAt || fields.blockClosingFor.isZero()) {
            return FLAlertLayer::keyBackClicked();
        }

        auto expiry = fields.shownAt.value() + fields.blockClosingFor;

        // only let it go through if it expired already
        if (expiry.isPast()) {
            FLAlertLayer::keyBackClicked();
        }
    }
};
