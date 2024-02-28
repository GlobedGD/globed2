#pragma once
#include <defs/all.hpp>

using EditRoleCallbackFn = std::function<void(int role)>;

class AdminEditRolePopup : public geode::Popup<int, EditRoleCallbackFn> {
public:
    static constexpr float POPUP_WIDTH = 180.f;
    static constexpr float POPUP_HEIGHT = 120.f;
    static AdminEditRolePopup* create(int currentRole, EditRoleCallbackFn fn);

    static std::string roleToSprite(int roleId);

private:
    EditRoleCallbackFn callback;

    bool setup(int currentRole, EditRoleCallbackFn fn) override;
};
