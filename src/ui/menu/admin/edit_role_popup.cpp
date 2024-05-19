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
    this->setTitle("Edit user role");

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

        util::ui::rescaleToMatch(spr1, {22.f, 22.f});

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
            })
            .opacity(present ? 255 : 69)
            .parent(buttonLayout);
    }

    buttonLayout->updateLayout();

    // button to save
    Build<ButtonSprite>::create("Save", "goldFont.fnt", "GJ_button_01.png", 0.8f)
        .scale(0.6f)
        .intoMenuItem([this] {
            this->callback(this->roles);
        })
        .pos(sizes.centerBottom + CCPoint{0.f, 15.f})
        .intoNewParent(CCMenu::create())
        .pos(0.f, 0.f)
        .parent(m_mainLayer);

    return true;
}

AdminEditRolePopup* AdminEditRolePopup::create(const std::vector<std::string>& roles, EditRoleCallbackFn fn) {
    auto ret = new AdminEditRolePopup;
    if (ret->init(POPUP_WIDTH, POPUP_HEIGHT, roles, fn)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}