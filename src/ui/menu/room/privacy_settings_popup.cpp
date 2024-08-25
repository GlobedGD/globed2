#include "privacy_settings_popup.hpp"

#include <net/manager.hpp>
#include <managers/settings.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

enum class PSetting {
    HideFromLists,
    DisableInvites,
    HideInLevel,
    HideRoles
};

static bool& _getSetting(PSetting setting) {
    auto& s = GlobedSettings::get().globed;

    switch (setting) {
        case PSetting::HideFromLists: return s.isInvisible.ref();
        case PSetting::DisableInvites: return s.noInvites.ref();
        case PSetting::HideInLevel: return s.hideInGame.ref();
        case PSetting::HideRoles: return s.hideRoles.ref();
    }
}

static bool getToggled(PSetting setting) {
    return _getSetting(setting);
}

static void setToggled(PSetting setting, bool state) {
    _getSetting(setting) = state;
    GlobedSettings::get().save();
}

bool PrivacySettingsPopup::setup() {
    auto popupLayout = util::ui::getPopupLayoutAnchored(m_size);

    Build<CCMenu>::create()
        .layout(
            RowLayout::create()->setGap(7.5f)
        )
        .id("button-menu")
        .store(buttonMenu)
        .pos(popupLayout.center)
        .parent(m_mainLayer);


    this->addButton(PSetting::HideFromLists, "button-privacy-lists-on.png"_spr, "button-privacy-lists-off.png"_spr);
    this->addButton(PSetting::DisableInvites, "button-privacy-invites-on.png"_spr, "button-privacy-invites-off.png"_spr);
    this->addButton(PSetting::HideInLevel, "button-privacy-ingame-on.png"_spr, "button-privacy-ingame-off.png"_spr);
    this->addButton(PSetting::HideRoles, "button-privacy-roles-on.png"_spr, "button-privacy-roles-off.png"_spr);

    buttonMenu->updateLayout();

    return true;
}

void PrivacySettingsPopup::addButton(PSetting setting, const char* onSprite, const char* offSprite) {
    auto toggler = CCMenuItemExt::createToggler(
        CCSprite::createWithSpriteFrameName(onSprite),
        CCSprite::createWithSpriteFrameName(offSprite),
        [this, setting](auto* btn) {
            bool enabled = btn->isToggled();

            setToggled(setting, enabled);
            this->sendPacket();
        }
    );

    toggler->toggle(!getToggled(setting));

    Build(toggler)
        .parent(buttonMenu);
}

void PrivacySettingsPopup::sendPacket() {
    auto flags = GlobedSettings::get().getPrivacyFlags();
    log::debug("sending flags: lists {}, invites {}, game {}, roles {}", flags.hideFromLists, flags.noInvites, flags.hideInGame, flags.hideRoles);
    NetworkManager::get().sendUpdatePlayerStatus(flags);
}

PrivacySettingsPopup* PrivacySettingsPopup::create() {
    auto ret = new PrivacySettingsPopup;
    if (ret->initAnchored(240.f, 120.f)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}