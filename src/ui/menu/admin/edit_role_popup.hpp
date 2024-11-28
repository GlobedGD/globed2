#pragma once
#include <defs/all.hpp>

class AdminUserPopup;

class AdminEditRolePopup : public geode::Popup<AdminUserPopup*, int32_t, const std::vector<std::string>&> {
public:
    static constexpr float POPUP_HEIGHT = 120.f;
    static AdminEditRolePopup* create(AdminUserPopup* parentPopup, int32_t accountId, const std::vector<std::string>& roles);

private:
    std::vector<std::string> roles;
    int32_t accountId;
    AdminUserPopup* parentPopup;

    bool setup(AdminUserPopup* parentPopup, int32_t accountId, const std::vector<std::string>& roles) override;
    void submit();
};
