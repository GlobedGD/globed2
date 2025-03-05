#pragma once
#include <defs/geode.hpp>

#include <Geode/modify/ProfilePage.hpp>

struct GLOBED_DLL GlobedProfilePage : geode::Modify<GlobedProfilePage, ProfilePage> {
	void loadPageFromUserInfo(GJUserScore*);
};
