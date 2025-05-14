#include "keybind_settings_popup.hpp"

#include "keybind_setup_popup.hpp"
#include <ui/general/list/list.hpp>
#include <managers/settings.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

namespace {
    class KeybindCell : public CCLayer {
        enumKeyCodes* settingPtr;
        enumKeyCodes settingDefault;
        CCMenu* menu;
        CCNode* keyWrapper = nullptr;

    public:
        bool init(const char* name, const char* description, enumKeyCodes* ptr, enumKeyCodes defaultv) {
            this->settingPtr = ptr;
            this->settingDefault = defaultv;

            float width = KeybindSettingsPopup::LIST_WIDTH;
            float height = KeybindSettingsPopup::CELL_HEIGHT;

            Build<CCLabelBMFont>::create(name, "goldFont.fnt")
                .scale(0.6f)
                .pos(6.f, height / 2.f)
                .anchorPoint(0.f, 0.5f)
                .parent(this);

            Build<CCMenu>::create()
                .layout(
                    RowLayout::create()
                        ->setAxisReverse(true)
                        ->setAxisAlignment(AxisAlignment::End)
                        ->setAutoScale(false)
                )
                .contentSize(width, height)
                .anchorPoint(0.f, 0.f)
                .pos(-6.f, 2.f)
                .parent(this)
                .store(menu);

            auto spr = Build<CCSprite>::createSpriteName("pencil.png"_spr)
                .collect();

            Build<CircleButtonSprite>::create(spr)
                .with([&](auto* item) {
                    util::ui::rescaleToMatch(item, {height * 0.7f, height * 0.7f});
                })
                .intoMenuItem([this](auto) {
                    KeybindSetupPopup::create(*this->settingPtr, [this](auto key) {
                        *this->settingPtr = key;
                        this->updateKey(key);
                        KeybindsManager::get().refreshBinds();
                        GlobedSettings::get().save();
                    })->show();
                })
                .parent(menu);

            menu->updateLayout();

            this->updateKey(*ptr);

            this->setContentSize({width, height});
            return true;
        }

        void updateKey(enumKeyCodes key) {
            auto& km = KeybindsManager::get();
            auto formatted = globed::formatKey(km.convertCocosKey(key));

            if (keyWrapper) {
                keyWrapper->removeFromParent();
            }

            float sqsize = KeybindSettingsPopup::CELL_HEIGHT * 0.75f;
            float sqscale = 2.f;

            auto node = Build<CCNode>::create()
                .parent(menu)
                .store(keyWrapper)
                .collect();

            CCLabelBMFont* label = Build<CCLabelBMFont>::create(formatted, "goldFont.fnt")
                .scale(0.63f)
                .zOrder(2)
                .parent(node)
                .collect();

            float width = label->getScaledContentWidth() + 6.f;

            label->setPosition({width / 2.f + 1.f, sqsize / 2.f + 1.f});

            auto bg = Build<CCScale9Sprite>::create("square02_small.png")
                .contentSize(width * sqscale, sqsize * sqscale)
                .scaleX(1.f / sqscale)
                .scaleY(1.f / sqscale)
                .opacity(80)
                .pos(width / 2.f, sqsize / 2.f)
                .parent(node)
                .collect();

            node->setContentSize(bg->getScaledContentSize());
            menu->updateLayout();
        }

        static KeybindCell* create(const char* name, const char* description, auto& setting) {
            auto ret = new KeybindCell;
            if (ret->init(name, description, setting.ptr(), setting.Default)) {
                ret->autorelease();
                return ret;
            }

            delete ret;
            return nullptr;
        }
    };
}

bool KeybindSettingsPopup::setup() {
    this->setTitle("Keybind Settings");

    KeybindsManager::get().refreshBinds();

    auto rlayout = util::ui::getPopupLayoutAnchored(m_size);

    auto list = Build(GlobedListLayer<KeybindCell>::createForComments(LIST_WIDTH, LIST_HEIGHT))
        .pos(rlayout.fromCenter(0.f, -10.f))
        .parent(m_mainLayer)
        .collect();

    auto& keys = GlobedSettings::get().keys;

#ifdef GLOBED_VOICE_CAN_TALK
    list->addCell(KeybindCell::create(
        "Voice activate",
        "Captures microphone input and sends it to other users while the button is held",
        keys.voiceChatKey
    ));

    list->addCell(KeybindCell::create(
        "Deafen",
        "Captures microphone input and sends it to other users while the button is held",
        keys.voiceDeafenKey
    ));
#endif

    list->addCell(KeybindCell::create(
        "Hide players",
        "Hide / unhide all players while in a level",
        keys.hidePlayersKey
    ));

    return true;
}

KeybindSettingsPopup* KeybindSettingsPopup::create() {
    auto ret = new KeybindSettingsPopup;
    if (ret->initAnchored(POPUP_WIDTH, POPUP_HEIGHT)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}
