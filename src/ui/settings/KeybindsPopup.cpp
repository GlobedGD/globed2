#include "KeybindsPopup.hpp"
#include "KeybindSetupPopup.hpp"
#include <globed/util/CStr.hpp>
#include <globed/core/SettingsManager.hpp>
#include <globed/core/KeybindsManager.hpp>

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {
const CCSize KeybindsPopup::POPUP_SIZE { 400.f, 280.f };
static constexpr CCSize LIST_SIZE { 340.f, 220.f };
static constexpr float CELL_HEIGHT = 26.f;

namespace {
class KeybindCell : public CCNode {
public:
    static KeybindCell* create(CStr key, CStr name, CStr desc) {
        auto ret = new KeybindCell;
        ret->init(key, name, desc);
        ret->autorelease();
        return ret;
    }

protected:
    CCMenu* m_menu;
    CCNode* m_keyWrapper = nullptr;
    CStr m_key;

    void init(CStr key, CStr name, CStr desc) {
        m_key = key;

        Build<CCLabelBMFont>::create(name, "goldFont.fnt")
            .scale(0.6f)
            .pos(6.f, CELL_HEIGHT / 2.f)
            .anchorPoint(0.f, 0.5f)
            .parent(this);

        m_menu = Build<CCMenu>::create()
            .layout(
                RowLayout::create()
                    ->setAxisReverse(true)
                    ->setAxisAlignment(AxisAlignment::End)
                    ->setAutoScale(false)
            )
            .contentSize(LIST_SIZE.width, CELL_HEIGHT)
            .anchorPoint(0.f, 0.f)
            .pos(-6.f, 2.f)
            .parent(this);

        auto spr = Build<CCSprite>::create("pencil.png"_spr)
            .collect();

        Build<CircleButtonSprite>::create(spr)
            .with([&](auto* item) {
                cue::rescaleToMatch(item, CELL_HEIGHT * 0.7f);
            })
            .intoMenuItem([this](auto) {
                auto popup = KeybindSetupPopup::create(this->getKey());
                popup->setCallback([this](auto key) {
                    this->setKey(key);
                });
                popup->show();
            })
            .parent(m_menu);

        m_menu->updateLayout();

        this->updateKey(this->getKey());

        this->setContentSize({LIST_SIZE.width, CELL_HEIGHT});
    }

    enumKeyCodes getKey() {
        return (enumKeyCodes)(int)globed::setting<int>(m_key);
    }

    void setKey(enumKeyCodes key) {
        globed::setting<int>(m_key) = (int)key;
        this->updateKey(key);
        KeybindsManager::get().refreshBinds();
    }

    void updateKey(enumKeyCodes key) {
        auto& km = KeybindsManager::get();
        auto formatted = globed::formatKey(convertCocosKey(key));

        cue::resetNode(m_keyWrapper);

        float sqSize = CELL_HEIGHT * 0.75f;
        float sqscale = 2.f;

        m_keyWrapper = Build<CCNode>::create()
            .parent(m_menu);

        CCLabelBMFont* label = Build<CCLabelBMFont>::create(formatted, "goldFont.fnt")
            .scale(0.63f)
            .zOrder(2)
            .parent(m_keyWrapper)
            .collect();

        float width = label->getScaledContentWidth() + 6.f;

        label->setPosition({width / 2.f + 1.f, sqSize / 2.f + 1.f});

        auto bg = Build<CCScale9Sprite>::create("square02_small.png")
            .contentSize(width * sqscale, sqSize * sqscale)
            .scaleX(1.f / sqscale)
            .scaleY(1.f / sqscale)
            .opacity(80)
            .pos(width / 2.f, sqSize / 2.f)
            .parent(m_keyWrapper)
            .collect();

        m_keyWrapper->setContentSize(bg->getScaledContentSize());
        m_menu->updateLayout();
    }
};
}

bool KeybindsPopup::setup() {
    this->setTitle("Keybind Settings");

    KeybindsManager::get().refreshBinds();

    m_list = Build(cue::ListNode::create(LIST_SIZE))
        .pos(this->fromCenter(0.f, -10.f))
        .parent(m_mainLayer);

#ifdef GLOBED_VOICE_CAN_TALK
    m_list->addCell(KeybindCell::create(
        "core.keybinds.voice-chat",
        "Voice activate",
        "Captures microphone input and sends it to other users while the button is held"
    ));

    m_list->addCell(KeybindCell::create(
        "core.keybinds.deafen",
        "Deafen",
        "Temporarily disables ability to hear other players"
    ));
#endif

    m_list->addCell(KeybindCell::create(
        "core.keybinds.hide-players",
        "Hide players",
        "Hide / unhide all players while in a level"
    ));

    return true;
}

}