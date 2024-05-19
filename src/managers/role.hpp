#pragma once

#include <defs/util.hpp>
#include <data/types/user.hpp>


class RoleManager : public SingletonBase<RoleManager> {
    friend class SingletonBase;

public:
    void setAllRoles(std::vector<GameServerRole>&& allRoles);
    void setAllRoles(const std::vector<GameServerRole>& allRoles);
    void clearAllRoles();
    std::vector<GameServerRole>& getAllRoles();
    ComputedRole compute(const std::vector<uint8_t>& roles);

private:
    std::vector<GameServerRole> allRoles;
};
