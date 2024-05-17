#pragma once

#include <defs/util.hpp>
#include <data/types/user.hpp>

#include <asp/sync/Atomic.hpp>

class AdminManager : public SingletonBase<AdminManager> {
    friend class SingletonBase;

public:
    bool authorized();
    void setAuthorized(ComputedRole&& role, std::vector<ServerRole>&& allRoles);
    void deauthorize();
    ComputedRole& getRole();
    std::vector<ServerRole>& getAllRoles();

private:
    asp::sync::AtomicBool authorized_;
    ComputedRole role = {};
    std::vector<ServerRole> allRoles;
};
