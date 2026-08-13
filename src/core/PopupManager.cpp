#include <globed/core/PopupManager.hpp>
#include <globed/util/CCData.hpp>
#include <core/hooks/GJBaseGameLayer.hpp>
#include <ui/BasePopup.hpp>
#include <ui/Emojis.hpp>

#include <Geode/ui/Label.hpp>
#include <UIBuilder.hpp>
#include <asp/time/SystemTime.hpp>
#include <asp/data/Cow.hpp>

using namespace geode::prelude;
using namespace asp::time;
using namespace asp::data;

namespace globed {

// CustomFLAlert

class CustomFLAlert : public BasePopup {
public:
    using Callback = geode::Function<void (FLAlertLayer*, bool)>;

    static CustomFLAlert* create(ZStringView title, std::string_view content, ZStringView btn1, ZStringView btn2, float width);

    void setCallback(Callback&& cb) {
        m_callback = std::move(cb);
    }

private:
    Callback m_callback;

    bool init(CCSize size, ZStringView title, ZStringView btn1, ZStringView btn2, CCNode* content);
    void onClick(bool btn2);
};

bool CustomFLAlert::init(CCSize size, ZStringView title, ZStringView btn1, ZStringView btn2, CCNode* content) {
    if (!BasePopup::init(size, "square01_001.png")) return false;

    m_closeBtn->setVisible(false);

    content->setPosition(m_size.width / 2.f, m_size.height / 2.f + 5.f);
    m_mainLayer->addChild(content);

    this->setTitle(title, "goldFont.fnt", 0.9f, 27.f);

    // confirm / cancel buttons
    auto bottomMenu = Build<CCMenu>::create()
        .layout(RowLayout::create()->setGap(15.f)->setAutoScale(false))
        .contentSize(m_size.width, 60.f)
        .pos(this->fromBottom(30.f))
        .parent(m_mainLayer)
        .collect();

    Build<ButtonSprite>::create(btn1.c_str(), "goldFont.fnt", "GJ_button_01.png", 1.0f)
        .intoMenuItem([this] {
            this->onClick(false);
        })
        .scaleMult(1.15f)
        .parent(bottomMenu);

    if (!btn2.empty()) {
        Build<ButtonSprite>::create(btn2.c_str(), "goldFont.fnt", "GJ_button_01.png", 1.0f)
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
    ZStringView title,
    std::string_view content,
    ZStringView btn1,
    ZStringView btn2,
    float rWidth
) {
    auto label = Label::createRich("", "chatFont.fnt");
    label->setAlignment(Label::Alignment::Center);
    label->setMaxWidth(rWidth - 20.f);
    label->setLineSpacing(3.f);

    std::string cont = std::string(content);
    if (globed::containsEmoji(content)) {
        globed::translateEmojiString(cont);
        label->setEmojiRegistry(getEmojiMap());
    }
    label->setRichText(std::move(cont));

    CCSize size = label->getScaledContentSize() + CCSize{24.f, 100.f};

    float width = std::max<float>(size.width, std::max<float>(350.f, rWidth));
    float height = std::max<float>(size.height, 140.f);

    auto ret = new CustomFLAlert();
    if (ret->init({width, height}, title, btn1, btn2, label)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

// PopupManager

PopupRef PopupManager::alert(
    ZStringView title,
    const std::string& content,
    ZStringView btn1,
    ZStringView btn2,
    float width
) {
    return this->quickPopup(title, content, btn1, btn2, {}, width);
}

PopupRef PopupManager::quickPopup(
    ZStringView title,
    const std::string& content,
    ZStringView btn1,
    ZStringView btn2,
    geode::Function<void (FLAlertLayer*, bool)> callback,
    float width
) {
    auto alert = CustomFLAlert::create(title, content, btn1, btn2.empty() ? nullptr : btn2.c_str(), width);

    if (callback) {
        alert->setCallback(std::move(callback));
    }

    return this->manage(alert);
}

PopupRef PopupManager::manage(FLAlertLayer* alert) {
    return geode::PopupManager::get().manage(alert);
}

bool PopupManager::isManaged(FLAlertLayer* alert) {
    return geode::PopupManager::get().isManaged(alert);
}

bool PopupManager::hasPendingPopups() const {
    return geode::PopupManager::get().hasPendingPopups();
}

void toast(geode::NotificationIcon icon, float duration, const std::string& message) {
    Notification::create(message, icon, duration)->show();
}

void toast(cocos2d::CCSprite* icon, float duration, const std::string& message) {
    Notification::create(message, icon, duration)->show();
}

}