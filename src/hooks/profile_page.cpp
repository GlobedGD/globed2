#include "profile_page.hpp"

#ifndef GLOBED_LESS_BINDINGS

#include <managers/admin.hpp>
#include <net/manager.hpp>

using namespace geode::prelude;

void GlobedProfilePage::loadPageFromUserInfo(GJUserScore* score) {
	ProfilePage::loadPageFromUserInfo(score);

	if (!AdminManager::get().canModerate()) return;

	auto leftMenu = static_cast<CCMenu*>(this->getChildByIDRecursive("left-menu"));

	// don't add if the menu doesn't exist, or if the button already exists
	if (!leftMenu || leftMenu->getChildByID("admin-button"_spr)) {
		return;
	}

	PlayerIconData iconData{};
	PlayerAccountData player {
		score->m_accountID,
		score->m_userID,
		score->m_userName,
		iconData
	};

	Build<CCSprite>::createSpriteName("GJ_reportBtn_001.png")
		.scale(.7f)
		.intoMenuItem([player](auto btn) {
			auto& nm = NetworkManager::get();
			if (!nm.established()) {
				auto parent = btn->getParent();
				btn->removeFromParent();
				parent->updateLayout();
				return;
			}

			AdminManager::get().openUserPopup(player.makeRoomPreview(0));
		})
		.parent(leftMenu)
		.id("admin-button"_spr);

	leftMenu->updateLayout();
}

#endif // GLOBED_LESS_BINDINGS
