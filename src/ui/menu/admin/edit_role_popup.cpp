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
        Build<CCSprite>::createSpriteName(roleToSprite(id).c_str())
            .scale(0.65f)
            .intoMenuItem([this, id = id](auto) {
                this->callback(id);
                this->onClose(this);
            })
            .parent(buttonLayout);
    }

    buttonLayout->updateLayout();

    return true;
}

std::string AdminEditRolePopup::roleToSprite(int roleId) {
    std::string btnSprite;

    switch (roleId) {
    case ROLE_USER: btnSprite = "role-user.png"_spr; break;
    case ROLE_HELPER: btnSprite = "role-helper.png"_spr; break;
    case ROLE_MOD: btnSprite = "role-mod.png"_spr; break;
    case ROLE_ADMIN: btnSprite = "role-admin.png"_spr; break;
    default: btnSprite = "role-user.png"_spr; break;
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