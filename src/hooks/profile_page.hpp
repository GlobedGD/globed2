#pragma once
#include <defs/geode.hpp>

#ifndef GLOBED_DISABLE_EXTRA_HOOKS

#include <Geode/modify/ProfilePage.hpp>

struct GLOBED_DLL GlobedProfilePage : geode::Modify<GlobedProfilePage, ProfilePage> {
	void loadPageFromUserInfo(GJUserScore*);
};

#endif // GLOBED_DISABLE_EXTRA_HOOKS
