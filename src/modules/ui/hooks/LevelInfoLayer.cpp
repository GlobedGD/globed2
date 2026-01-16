#include "Common.hpp"
#include <core/hooks/GJBaseGameLayer.hpp>
#include <core/net/NetworkManagerImpl.hpp>
#include <globed/config.hpp>
#include <globed/core/RoomManager.hpp>
#include <globed/core/actions.hpp>
#include <globed/util/gd.hpp>
#include <modules/ui/UIModule.hpp>
#include <ui/menu/FeatureCommon.hpp>
#include <ui/menu/FeaturedListLayer.hpp>
#include <ui/menu/SendFeaturePopup.hpp>
#include <ui/menu/levels/LevelListLayer.hpp>

#include <Geode/Geode.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

struct GLOBED_MODIFY_ATTR HookedLevelInfoLayer : geode::Modify<HookedLevelInfoLayer, LevelInfoLayer> {
    struct Fields {
        bool m_allowAnyway = false;
        std::optional<FeatureTier> m_attachedTier;
    };

    static void onModify(auto &self)
    {
        GLOBED_CLAIM_HOOKS(UIModule::get(), self, "LevelInfoLayer::init", "LevelInfoLayer::onBack",
                           "LevelInfoLayer::onPlay", "LevelInfoLayer::playStep3", "LevelInfoLayer::tryCloneLevel", );
    }

    $override bool init(GJGameLevel *level, bool challenge)
    {
        if (!LevelInfoLayer::init(level, challenge)) {
            return false;
        }

        auto &nm = NetworkManagerImpl::get();
        auto &rm = RoomManager::get();

        if (nm.isAuthorizedModerator() && nm.getUserPermissions().canSendFeatures) {
            this->addLevelSendButton();
        }

        // i hate myself
        if (level->m_levelIndex == 0) {
            for (auto child : CCArrayExt<CCNode *>(CCScene::get()->getChildren())) {
                if (typeinfo_cast<LevelListLayer *>(child) || typeinfo_cast<FeaturedListLayer *>(child)) {
                    globed::reorderDownloadedLevel(level);
                    break;
                }
            }
        }

        if (rm.isOwner() && rm.getSettings().manualPinning) {
            this->addPinButton();
        }

        if (auto rating = featureTierFromLevel(level)) {
            m_fields->m_attachedTier = *rating;
            globed::findAndAttachRatingSprite(this, *rating);
        }

        return true;
    }

    void addLevelSendButton()
    {
        auto *leftMenu = typeinfo_cast<CCMenu *>(this->getChildByIDRecursive("left-side-menu"));
        if (!leftMenu) {
            return;
        }

        bool plat = m_level->isPlatformer();

        Build<CCSprite>::create("icon-send-btn.png"_spr)
            .with([&](auto spr) { cue::rescaleToMatch(spr, 46.f); })
            .intoMenuItem([this, plat] {
                if (plat) {
                    SendFeaturePopup::create(m_level)->show();
                } else {
                    PopupManager::get()
                        .alert("Error", "Only <cj>Platformer levels</c> are eligible to be <cg>Globed Featured!</c>")
                        .showInstant();
                }
            })
            .with([&](auto *btn) {
                if (!plat) {
                    btn->setColor({100, 100, 100});
                }
            })
            .id("send-btn"_spr)
            .parent(leftMenu);

        leftMenu->updateLayout();
    }

    void addPinButton()
    {
        auto rightMenu = this->getChildByIDRecursive("right-side-menu");
        if (!rightMenu)
            return;

        Build<CCSprite>::create("button-pin-level.png"_spr)
            .intoMenuItem([this] {
                globed::confirmPopup("Globed",
                                     "Are you sure you want to <cy>pin</c> this level to the <cg>current room</c>?",
                                     "Cancel", "Yes", [this](auto *) {
                                         NetworkManagerImpl::get().sendUpdatePinnedLevel(
                                             RoomManager::get().makeSessionId(m_level->m_levelID).asU64());
                                     });
            })
            .id("pin-level-btn"_spr)
            .parent(rightMenu);

        rightMenu->updateLayout();
    }

    $override void onBack(CCObject *sender)
    {
        // clear warp context
        if (globed::_getWarpContext().levelId() == m_level->m_levelID) {
            globed::_clearWarpContext();
        }

        LevelInfoLayer::onBack(sender);
    }

    $override void onPlay(CCObject *s)
    {
        if (m_fields->m_allowAnyway) {
            m_fields->m_allowAnyway = false;
            LevelInfoLayer::onPlay(s);
            return;
        }

        // don't allow joining when in a follower room
        if (disallowLevelJoin(m_level->m_levelID)) {
            return;
        }

        // don't allow joining if already in a level
        if (NetworkManagerImpl::get().isConnected() && GJBaseGameLayer::get() != nullptr) {
            globed::confirmPopup("Warning",
                                 "You are already inside of a level, opening another level may cause Globed to "
                                 "<cr>crash</c>. <cy>Do you still want to proceed?</c>",
                                 "Cancel", "Play", [this, s](auto) { this->forcePlay(s); });
            return;
        }

        LevelInfoLayer::onPlay(s);
    }

    $override void playStep3()
    {
        if (auto tier = m_fields->m_attachedTier) {
            globed::setFeatureTierForLevel(m_level, *tier);
        }

        LevelInfoLayer::playStep3();
    }

    void forcePlay(CCObject *s)
    {
        m_fields->m_allowAnyway = true;
        LevelInfoLayer::onPlay(s);
    }

    $override void tryCloneLevel(CCObject *s)
    {
        if (NetworkManagerImpl::get().isConnected() && GJBaseGameLayer::get() != nullptr) {
            globed::alert("Globed Error", "Cannot perform this action while in a level");
            return;
        }

        LevelInfoLayer::tryCloneLevel(s);
    }
};

} // namespace globed