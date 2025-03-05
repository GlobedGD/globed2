#include "profile_page.hpp"

#include <managers/admin.hpp>

using namespace geode::prelude;

void GlobedProfilePage::loadPageFromUserInfo(GJUserScore* score) {
	ProfilePage::loadPageFromUserInfo(score);

	if (!AdminManager::get().canModerate()) return;

	if (!this->getChildByIDRecursive("left-menu")) return;

	auto leftMenu = static_cast<CCMenu*>(this->getChildByIDRecursive("left-menu"));

	auto iconData = PlayerIconData {};
	auto player = PlayerAccountData {
		score->m_accountID,
		score->m_userID,
		score->m_userName,
		iconData
	};

	Build<CCSprite>::createSpriteName("GJ_reportBtn_001.png")
	.scale(.7f)
	.intoMenuItem([player](auto) {
		AdminManager::get().openUserPopup(player.makeRoomPreview(0));
	})
	.parent(leftMenu)
	.id("admin-button"_spr);

	leftMenu->updateLayout();
}