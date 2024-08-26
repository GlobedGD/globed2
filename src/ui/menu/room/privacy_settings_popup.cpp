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

    this->setTitle("Visibility Settings");

    Build<CCMenu>::create()
        .layout(
            RowLayout::create()->setGap(15.f)->setAutoScale(false)
        )
        .id("button-menu")
        .store(buttonMenu)
        .pos(popupLayout.center - CCPoint{0.f, 10.f})
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

    auto wrapperNode = Build<CCMenu>::create()
        .contentSize(toggler->getScaledContentSize())
        .child(toggler)
        .parent(buttonMenu)
        // info button
        .intoNewChild(CCMenu::create())
        .pos(30.f, 30.f)
        .intoNewChild(
            Build<CCSprite>::createSpriteName("GJ_infoIcon_001.png")
                .scale(0.5f)
                .intoMenuItem([this, setting] {
                    this->onDescriptionClicked(setting);
                })
        )
        .with([](auto* btn) {
            auto size = btn->getScaledContentSize();
            btn->getParent()->setContentSize(size);
            btn->setPosition(size / 2.f);
        })
        .collect();

    CCSize sz = toggler->getScaledContentSize();

    toggler->setPosition(sz / 2.f);
}

void PrivacySettingsPopup::onDescriptionClicked(PSetting setting) {

}

void PrivacySettingsPopup::sendPacket() {
    auto flags = GlobedSettings::get().getPrivacyFlags();

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