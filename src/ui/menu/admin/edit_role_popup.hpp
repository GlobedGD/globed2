#pragma once
#include <defs/all.hpp>

using EditRoleCallbackFn = std::function<void(const std::vector<std::string>& role)>;

class AdminEditRolePopup : public geode::Popup<const std::vector<std::string>&, EditRoleCallbackFn> {
public:
    static constexpr float POPUP_WIDTH = 180.f;
    static constexpr float POPUP_HEIGHT = 120.f;
    static AdminEditRolePopup* create(const std::vector<std::string>& roles, EditRoleCallbackFn fn);

    static std::string roleToSprite(int roleId);

private:
    EditRoleCallbackFn callback;
    std::vector<std::string> roles;

    bool setup(const std::vector<std::string>& roles, EditRoleCallbackFn fn) override;
};
