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
    using Callback = std23::move_only_function<void (FLAlertLayer*, bool)>;

    static CustomFLAlert* create(CStr title, std::string_view content, CStr btn1, CStr btn2, float width);

    void setCallback(Callback&& cb) {
        m_callback = std::move(cb);
    }

private:
    Callback m_callback;

    bool setup(CStr title, CStr btn1, CStr btn2, CCNode* content) override;
    void onClick(bool btn2);
};

bool CustomFLAlert::setup(CStr title, CStr btn1, CStr btn2, CCNode* content) {
    m_closeBtn->setVisible(false);

    content->setPosition(m_size.width / 2.f, m_size.height / 2.f + 5.f);
    m_mainLayer->addChild(content);

    this->setTitle(title.get(), "goldFont.fnt", 0.9f, 27.f);

    // confirm / cancel buttons
    auto bottomMenu = Build<CCMenu>::create()
        .layout(RowLayout::create()->setGap(15.f)->setAutoScale(false))
        .contentSize(m_size.width, 60.f)
        .pos(this->fromBottom(30.f))
        .parent(m_mainLayer)
        .collect();

    Build<ButtonSprite>::create(btn1, "goldFont.fnt", "GJ_button_01.png", 1.0f)
        .intoMenuItem([this] {
            this->onClick(false);
        })
        .scaleMult(1.15f)
        .parent(bottomMenu);

    if (btn2) {
        Build<ButtonSprite>::create(btn2, "goldFont.fnt", "GJ_button_01.png", 1.0f)
            .intoMenuItem([this] {
                this->onClick(true);
            })
            .scaleMult(1.15f)
            .parent(bottomMenu);
    }

    bottomMenu->updateLayout();

    return true;
}

void CustomFLAlert::onClick(bool btn2) {
    if (m_callback) {
        m_callback(this, btn2);
    }

    this->onClose(nullptr);
}

CustomFLAlert* CustomFLAlert::create(
    CStr title,
    std::string_view content,
    CStr btn1,
    CStr btn2,
    float rWidth
) {
    auto label = Label::createWrapped("", "chatFont.fnt", BMFontAlignment::Center, rWidth - 20.f);
    label->setExtraLineSpacing(3.f);

    if (globed::containsEmoji(content)) {
        std::string cont = std::string(content);
        globed::translateEmojiString(cont);

        label->enableEmojis("twemojis.png"_spr, getEmojiMap());
        label->setString(cont);
    } else {
        // no emojis!
        globed::colorizeLabel(label, content);
    }

    CCSize size = label->getScaledContentSize() + CCSize{24.f, 100.f};

    float width = std::max<float>(size.width, std::max<float>(350.f, rWidth));
    float height = std::max<float>(size.height, 140.f);

    auto ret = new CustomFLAlert();
    if (ret->initAnchored(width, height, title, btn1, btn2, label, "square01_001.png", {0.f, 0.f, 94.f, 94.f})) {
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
    std23::move_only_function<void (FLAlertLayer*, bool)> callback,
    float width
) {
    auto alert = CustomFLAlert::create(title, content, btn1, btn2.getOrNull(), width);

    if (callback) {
        alert->setCallback(std::move(callback));
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

bool PopupManager::hasPendingPopups() const {
    return !m_queuedPopups.empty();
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

static ccColor3B mapColor(char c) {
    switch (c) {
        case 'b': return color3FromHex("#4a52e1");
        case 'g': return color3FromHex("#40e348");
        case 'l': return color3FromHex("#60abef");
        case 'j': return color3FromHex("#32c8ff");
        case 'y': return color3FromHex("#ffff00");
        case 'o': return color3FromHex("#ffa54b");
        case 'r': return color3FromHex("#ff5a5a");
        case 'p': return color3FromHex("#ff00ff");
        case 'a': return color3FromHex("#9632ff");
        case 'd': return color3FromHex("#ff96ff");
        case 'c': return color3FromHex("#ffff96");
        case 'f': return color3FromHex("#96ffff");
        case 's': return color3FromHex("#ffdc41");
        default: return color3FromHex("#ff0000");
    }
}

void colorizeLabel(Label* label, std::string_view text) {
    struct Run {
        size_t start, end;
        ccColor3B color;
    };

    std::string outText;
    std::vector<Run> runs;
    size_t i = 0;
    size_t childI = 0;

    auto skip = [&](size_t n = 1) {
        auto sv = text.substr(i, n);
        outText += sv;
        i += n;

        for (char c : sv) {
            if (c != ' ' && c != '\t' && c != '\n' && c != '\r') childI++;
        }
    };

    std::optional<Run> current;

    while ((int64_t)text.size() - (int64_t)i >= 4) {
        std::string_view slice = text.substr(i, 4);

        if (!current) {
            // Scan for beginning of a new run
            if (!slice.starts_with("<c") || slice[3] != '>') {
                skip();
                continue;
            }

            auto color = mapColor(slice[2]);
            current = Run{childI, 0, color};
            i += 4;
            continue;
        }

        // Scan for end of current run
        if (slice != "</c>") {
            skip();
            continue;
        }

        current->end = childI;
        runs.push_back(*current);
        current = std::nullopt;
        i += 4;
    }

    // append remaining text
    skip(text.size() - i);

    label->setString(outText);
    auto mainBatch = label->getChildByType<CCSpriteBatchNode>(0);
    auto mchildren = mainBatch->getChildrenExt<CCSprite>();

    for (auto& run : runs) {
        for (size_t i = run.start; i < run.end; i++) {
            mchildren[i]->setColor(run.color);
        }
    }
}

}