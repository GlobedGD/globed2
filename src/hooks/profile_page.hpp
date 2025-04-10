#pragma once
#include <defs/geode.hpp>

#ifndef GLOBED_LESS_BINDINGS

#include <Geode/modify/ProfilePage.hpp>

struct GLOBED_DLL GlobedProfilePage : geode::Modify<GlobedProfilePage, ProfilePage> {
	void loadPageFromUserInfo(GJUserScore*);
};

#endif // GLOBED_LESS_BINDINGS
