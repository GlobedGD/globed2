#include "edit_role_popup.hpp"

#include <managers/admin.hpp>
#include <managers/role.hpp>
#include <util/ui.hpp>
#include <util/cocos.hpp>

using namespace geode::prelude;

static GameServerRole* findRole(const std::string& id) {
    auto& all = RoleManager::get().getAllRoles();

    for (auto& role : all) {
        if (role.role.id == id) return &role;
    }

    return nullptr;
}

bool AdminEditRolePopup::setup(const std::vector<std::string>& roles, EditRoleCallbackFn callback) {
    this->callback = callback;
    this->roles = roles;
    this->setTitle("Edit user roles");

    auto sizes = util::ui::getPopupLayout(m_size);

    auto* buttonLayout = Build<CCMenu>::create()
        .pos(sizes.center)
        .layout(RowLayout::create()->setGap(3.f))
        .parent(m_mainLayer)
        .collect();

    auto& allRoles = RoleManager::get().getAllRoles();
    for (const auto& role : allRoles) {
        auto* spr1 = util::ui::createBadge(role.role.badgeIcon);
        if (!spr1) continue;

        bool present = std::find_if(roles.begin(), roles.end(), [&](const std::string& k) { return k == role.role.id; }) != roles.end();

        util::ui::rescaleToMatch(spr1, util::ui::BADGE_SIZE);

        Build(spr1)
            .intoMenuItem([this, roleid = role.role.id](auto btn) {
                auto it = std::find_if(this->roles.begin(), this->roles.end(), [&](const std::string& k) { return k == roleid; });

                bool present = it != this->roles.end();

                if (present) {
                    btn->setOpacity(69);
                    this->roles.erase(it);
                } else {
                    btn->setOpacity(255);
                    this->roles.push_back(roleid);
                }

                this->callback(this->roles);
            })
            // order more priority roles to be on the left
            .zOrder(-role.role.priority)
            .opacity(present ? 255 : 69)
            .parent(buttonLayout);
    }

    buttonLayout->updateLayout();

    return true;
}

AdminEditRolePopup* AdminEditRolePopup::create(const std::vector<std::string>& roles, EditRoleCallbackFn fn) {
    auto ret = new AdminEditRolePopup;
    if (ret->init(WIDTH_PER_ROLE * RoleManager::get().getAllRoles().size(), POPUP_HEIGHT, roles, fn)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}