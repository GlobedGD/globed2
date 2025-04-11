#include "keybind_settings_popup.hpp"

#include <ui/general/list/list.hpp>
#include <managers/settings.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

namespace {
    class KeybindCell : public CCLayer {
    public:
        bool init(const char* name, const char* description, enumKeyCodes* ptr, enumKeyCodes defaultv) {
            Build<CCLabelBMFont>::create(name, "goldFont.fnt")
                .pos(4.f, KeybindSettingsPopup::CELL_HEIGHT / 2.f)
                .anchorPoint(0.f, 0.5f)
                .parent(this);

            this->setContentSize({KeybindSettingsPopup::LIST_WIDTH, KeybindSettingsPopup::CELL_HEIGHT});
            return true;
        }

        static KeybindCell* create(const char* name, const char* description, auto setting) {
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
