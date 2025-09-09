#include "ModRoleModifyPopup.hpp"
#include <globed/core/PopupManager.hpp>
#include <ui/misc/Badges.hpp>

#include <cue/Util.hpp>
#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

CCSize ModRoleModifyPopup::POPUP_SIZE {};

static const auto ROLE_ICON_SIZE = BADGE_SIZE * 1.2f;
static const float WIDTH_PER_ROLE = ROLE_ICON_SIZE.width + 3.f;

bool ModRoleModifyPopup::setup(int32_t accountId, std::vector<uint8_t> roleIds) {
    m_accountId = accountId;
    m_roleIds = std::move(roleIds);

    this->setTitle("Edit user roles");

    auto* buttonLayout = Build<CCMenu>::create()
        .pos(this->center())
        .layout(RowLayout::create()->setGap(3.f))
        .parent(m_mainLayer)
        .collect();

    auto allRoles = NetworkManagerImpl::get().getAllRoles();

    for (const auto& role : allRoles) {
        auto* spr1 = globed::createBadge(role.icon);
        if (!spr1) continue;

        bool present = std::find_if(m_roleIds.begin(), m_roleIds.end(), [&](uint8_t k) { return k == role.id; }) != m_roleIds.end();
        cue::rescaleToMatch(spr1, ROLE_ICON_SIZE);

        Build(spr1)
            .intoMenuItem([this, roleid = role.id](auto btn) {
                auto it = std::find_if(m_roleIds.begin(), m_roleIds.end(), [&](uint8_t k) { return k == roleid; });

                bool present = it != m_roleIds.end();

                if (present) {
                    btn->setOpacity(69);
                    m_roleIds.erase(it);
                } else {
                    btn->setOpacity(255);
                    m_roleIds.push_back(roleid);
                }
            })
            .opacity(present ? 255 : 69)
            .parent(buttonLayout);
    }

    Build<ButtonSprite>::create("Update", "bigFont.fnt", "GJ_button_01.png", 0.9f)
        .scale(0.8f)
        .intoMenuItem([this] {
            this->submit();
        })
        .pos(this->fromBottom(28.f))
        .intoNewParent(CCMenu::create())
        .pos(0.f, 0.f)
        .parent(m_mainLayer);

    buttonLayout->updateLayout();

    return true;
}

void ModRoleModifyPopup::setCallback(Callback&& cb) {
    m_callback = std::move(cb);
}

void ModRoleModifyPopup::submit() {
    this->startWaiting();
    NetworkManagerImpl::get().sendAdminEditRoles(m_accountId, m_roleIds);
}

void ModRoleModifyPopup::startWaiting() {
    m_listener = NetworkManagerImpl::get().listen<msg::AdminResultMessage>([this](const auto& msg) {
        this->stopWaiting(msg);
        return ListenerResult::Stop;
    });
    m_listener->setPriority(-100);

    if (m_loadPopup) {
        m_loadPopup->forceClose();
    }

    m_loadPopup = LoadingPopup::create();
    m_loadPopup->show();
}

void ModRoleModifyPopup::stopWaiting(const msg::AdminResultMessage& msg) {
    m_listener.reset();

    if (m_loadPopup) {
        m_loadPopup->forceClose();
        m_loadPopup = nullptr;
    }

    if (!msg.success) {
        globed::alertFormat("Error", "Failed to set user roles: <cy>{}</c>", msg.error);
    }

    if (m_callback) m_callback();

    this->onClose(nullptr);
}

ModRoleModifyPopup* ModRoleModifyPopup::create(int32_t accountId, std::vector<uint8_t> roleIds) {
    auto ret = new ModRoleModifyPopup;
    size_t allCnt = NetworkManagerImpl::get().getAllRoles().size();

    POPUP_SIZE = CCSize{std::max(150.f, 50.f + allCnt * WIDTH_PER_ROLE), 120.f};

    return BasePopup::create(accountId, std::move(roleIds));
}

}