#include "edit_role_popup.hpp"

#include <data/packets/client/admin.hpp>
#include <managers/admin.hpp>
#include <managers/role.hpp>
#include <net/manager.hpp>
#include <util/ui.hpp>
#include <util/cocos.hpp>

#include "user_popup.hpp"

using namespace geode::prelude;

static const auto ROLE_ICON_SIZE = util::ui::BADGE_SIZE * 1.2f;
static const float WIDTH_PER_ROLE = ROLE_ICON_SIZE.width + 3.f;

static GameServerRole* findRole(const std::string& id) {
    auto& all = RoleManager::get().getAllRoles();

    for (auto& role : all) {
        if (role.role.id == id) return &role;
    }

    return nullptr;
}

bool AdminEditRolePopup::setup(AdminUserPopup* parentPopup, int32_t accountId, const std::vector<std::string>& roles) {
    this->parentPopup = parentPopup;
    this->accountId = accountId;
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

        util::ui::rescaleToMatch(spr1, util::ui::BADGE_SIZE * 1.2f);

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
            // order more priority roles to be on the left
            .zOrder(-role.role.priority)
            .opacity(present ? 255 : 69)
            .parent(buttonLayout);
    }

    Build<ButtonSprite>::create("Update", "bigFont.fnt", "GJ_button_01.png", 0.9f)
        .scale(0.8f)
        .intoMenuItem([this] {
            this->submit();
        })
        .pos(sizes.fromBottom(28.f))
        .intoNewParent(CCMenu::create())
        .pos(0.f, 0.f)
        .parent(m_mainLayer);

    buttonLayout->updateLayout();

    return true;
}

void AdminEditRolePopup::submit() {
    NetworkManager::get().send(AdminSetUserRolesPacket::create(accountId, this->roles));
    this->onClose(nullptr);
    this->parentPopup->showLoadingPopup();
}

AdminEditRolePopup* AdminEditRolePopup::create(AdminUserPopup* parentPopup, int32_t accountId, const std::vector<std::string>& roles) {
    auto ret = new AdminEditRolePopup;
    if (ret->init(50.f + WIDTH_PER_ROLE * RoleManager::get().getAllRoles().size(), POPUP_HEIGHT, parentPopup, accountId, roles)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}