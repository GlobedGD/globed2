#include <core/net/NetworkManagerImpl.hpp>
#include <globed/core/actions.hpp>
#include <modules/ui/UIModule.hpp>

#include <Geode/Geode.hpp>
#include <Geode/modify/ProfilePage.hpp>
#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

struct GLOBED_MODIFY_ATTR HookedProfilePage : geode::Modify<HookedProfilePage, ProfilePage> {
    static void onModify(auto &self)
    {
        GLOBED_CLAIM_HOOKS(UIModule::get(), self, "ProfilePage::loadPageFromUserInfo", );
    }

    $override void loadPageFromUserInfo(GJUserScore *score)
    {
        ProfilePage::loadPageFromUserInfo(score);

        if (!NetworkManagerImpl::get().isAuthorizedModerator())
            return;

        auto leftMenu = static_cast<CCMenu *>(this->getChildByIDRecursive("left-menu"));

        // don't add if the menu doesn't exist, or if the button already exists
        if (!leftMenu || leftMenu->getChildByID("admin-button"_spr)) {
            return;
        }

        Build<CCSprite>::createSpriteName("GJ_reportBtn_001.png")
            .scale(.7f)
            .intoMenuItem([id = score->m_accountID](auto btn) { globed::openModPanel(id); })
            .parent(leftMenu)
            .id("admin-button"_spr);

        leftMenu->updateLayout();
    }
};

} // namespace globed
