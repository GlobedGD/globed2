#pragma once

#include <asp/sync/Atomic.hpp>

#include <data/types/user.hpp>
#include <data/types/gd.hpp>

#include <util/singleton.hpp>

class AdminManager : public SingletonBase<AdminManager> {
    friend class SingletonBase;

public:
    bool authorized();
    void setAuthorized(ComputedRole&& role);
    void deauthorize();
    ComputedRole& getRole();

    void openUserPopup(const PlayerRoomPreviewAccountData& rpdata);

private:
    asp::sync::AtomicBool authorized_;
    ComputedRole role = {};
};
