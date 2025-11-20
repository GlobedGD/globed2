#include "UserSettingsPopup.hpp"
#include <core/net/NetworkManagerImpl.hpp>

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

const CCSize UserSettingsPopup::POPUP_SIZE { 240.f, 120.f };

static void onDescriptionClicked(std::string_view key) {
    if (key == "core.user.hide-in-levels") {
        globed::alertFormat("Hide in Levels", "When enabled, other players won't be able to see you in levels.");
    } else if (key == "core.user.hide-in-menus") {
        globed::alertFormat("Hide in Menus", "When enabled, other players won't be able to see you in menus and level lists.");
    } else if (key == "core.user.hide-roles") {
        globed::alertFormat("Hide Roles", "When enabled, other players won't be able to see your special roles (e.g. Moderator) in levels and menus.");
    } else {
        globed::alertFormat("Info", "No description available.");
    }
}

bool UserSettingsPopup::setup() {
    this->setTitle("User Settings");

    m_menu = Build<CCMenu>::create()
        .layout(RowLayout::create()->setGap(12.f)->setAutoScale(false))
        .id("buttons-menu")
        .pos(this->fromCenter(0.f, -10.f))
        .parent(m_mainLayer);

    this->addButton("core.user.hide-in-levels", "privacy-ingame-off.png"_spr, "privacy-ingame-on.png"_spr);
    this->addButton("core.user.hide-in-menus", "privacy-lists-off.png"_spr, "privacy-lists-on.png"_spr);

    if (NetworkManagerImpl::get().isModerator()) {
        this->addButton("core.user.hide-roles", "privacy-roles-off.png"_spr, "privacy-roles-on.png"_spr);
    }

    m_menu->updateLayout();

    return true;
}

void UserSettingsPopup::addButton(std::string_view key, const char* onSprite, const char* offSprite) {
    auto toggler = CCMenuItemExt::createToggler(
        CCSprite::create(onSprite),
        CCSprite::create(offSprite),
        [this, key](auto btn) {
            if (key == "core.user.hide-in-levels" && !globed::swapFlag("core.flags.seen-hide-in-levels-note")) {
                globed::alertFormat(
                    "Warning",
                    "This setting will make you <cr>invisible to all players</c> until disabled."
                );
                btn->toggle(true);
                return;
            }

            m_modified = true;
            bool enabled = !btn->isToggled();
            globed::setting<bool>(key) = enabled;
        }
    );

    toggler->toggle(globed::setting<bool>(key));

    Build<CCMenu>::create()
        .id(std::string{key})
        .contentSize(toggler->getScaledContentSize())
        .child(toggler)
        .parent(m_menu)
        .intoNewChild(
            Build<CCSprite>::createSpriteName("GJ_infoIcon_001.png")
                .scale(0.5f)
                .intoMenuItem([key] {
                    onDescriptionClicked(key);
                })
                .pos(toggler->getScaledContentSize() + CCSize{1.f, 1.f})
        );

    toggler->setPosition(toggler->getScaledContentSize() / 2.f);
}

void UserSettingsPopup::onClose(CCObject* o) {
    bool modified = m_modified;
    Popup::onClose(o);

    if (!modified) return;

    auto& nm = NetworkManagerImpl::get();

    if (nm.isConnected()) {
        nm.sendUpdateUserSettings();
    }
}

}
