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

// CustomFLAlert

class CustomFLAlert : public BasePopup {
public:
    using Callback = geode::Function<void (FLAlertLayer*, bool)>;

    static CustomFLAlert* create(CStr title, std::string_view content, CStr btn1, CStr btn2, float width);

    void setCallback(Callback&& cb) {
        m_callback = std::move(cb);
    }

private:
    Callback m_callback;

    bool init(CCSize size, CStr title, CStr btn1, CStr btn2, CCNode* content);
    void onClick(bool btn2);
};

bool CustomFLAlert::init(CCSize size, CStr title, CStr btn1, CStr btn2, CCNode* content) {
    if (!BasePopup::init(size, "square01_001.png")) return false;

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
    if (ret->init({width, height}, title, btn1, btn2, label)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

// PopupManager

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
    geode::Function<void (FLAlertLayer*, bool)> callback,
    float width
) {
    auto alert = CustomFLAlert::create(title, content, btn1, btn2.getOrNull(), width);

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