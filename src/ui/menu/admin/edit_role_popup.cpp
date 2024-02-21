#include "edit_role_popup.hpp"

#include <net/network_manager.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

bool AdminEditRolePopup::setup(int currentRole, EditRoleCallbackFn callback) {
    this->callback = callback;
    this->setTitle("Edit user role");

    auto sizes = util::ui::getPopupLayout(m_size);

    auto* buttonLayout = Build<CCMenu>::create()
        .pos(sizes.center)
        .layout(RowLayout::create()->setGap(3.f))
        .parent(m_mainLayer)
        .collect();

    // the buttons themselves
    for (int id : {ROLE_USER, ROLE_HELPER, ROLE_MOD, ROLE_ADMIN}) {
        Build<CCSprite>::createSpriteName(roleToSprite(id))
            .intoMenuItem([this, id = id](auto) {
                this->callback(id);
                this->onClose(this);
            })
            .parent(buttonLayout);
    }

    buttonLayout->updateLayout();

    return true;
}

const char* AdminEditRolePopup::roleToSprite(int roleId) {
    const char* btnSprite;

    switch (roleId) {
    case ROLE_USER: btnSprite = "diffIcon_00_btn_001.png"; break;
    case ROLE_HELPER: btnSprite = "modBadge_03_001.png"; break;
    case ROLE_MOD: btnSprite = "modBadge_01_001.png"; break;
    case ROLE_ADMIN: btnSprite = "modBadge_02_001.png"; break;
    default: btnSprite = "diffIcon_00_btn_001.png"; break;
    }

    return btnSprite;
}

AdminEditRolePopup* AdminEditRolePopup::create(int currentRole, EditRoleCallbackFn fn) {
    auto ret = new AdminEditRolePopup;
    if (ret->init(POPUP_WIDTH, POPUP_HEIGHT, currentRole, fn)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}