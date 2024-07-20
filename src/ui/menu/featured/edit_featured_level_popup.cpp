#include "edit_featured_level_popup.hpp"

#include <Geode/ui/TextInput.hpp>

#include <managers/error_queues.hpp>
#include <managers/daily_manager.hpp>
#include <managers/admin.hpp>
#include <net/manager.hpp>
#include <data/packets/client/admin.hpp>
#include <util/ui.hpp>
#include <util/format.hpp>
#include <util/gd.hpp>

using namespace geode::prelude;

bool EditFeaturedLevelPopup::setup(GJGameLevel* level) {
    this->level = level;

    this->setTitle("Globed: Suggest Level", "goldFont.fnt", 0.9f, 20.f);

    auto rlayout = util::ui::getPopupLayoutAnchored(m_size);

    Build<CCMenu>::create()
        .pos(rlayout.fromTop(77.f))
        .parent(m_mainLayer)
        .store(menu);

    int diff = this->getDifficulty();

    Build<GJDifficultySprite>::create(diff, GJDifficultyName::Short)
        .pos(rlayout.fromTop(77.f))
        .parent(m_mainLayer)
        .scale(1.25)
        .zOrder(3);

    this->createDiffButton();

    Build<TextInput>::create(POPUP_WIDTH * 0.8f, "Notes", "chatFont.fnt")
        .pos(rlayout.fromBottom(60.f))
        .parent(m_mainLayer)
        .store(notesInput);

    notesInput->setCommonFilter(CommonFilter::Any);

    auto* buttonMenu = Build<CCMenu>::create()
        .layout(RowLayout::create())
        .contentSize(POPUP_WIDTH * 0.8f, 50.f)
        .pos(rlayout.fromBottom(27.f))
        .parent(m_mainLayer)
        .collect();

    Build<ButtonSprite>::create("Send", "bigFont.fnt", "GJ_button_01.png", 0.9f)
        .scale(0.75f)
        .intoMenuItem([this] {
            auto& nm = NetworkManager::get();

            nm.send(AdminSendFeaturedLevelPacket::create(
                this->level->m_levelName,
                this->level->m_levelID,
                this->level->m_creatorName,
                util::gd::calcLevelDifficulty(this->level),
                currIdx,
                this->notesInput->getString()
            ));

            Notification::create("Successfully sent level!", NotificationIcon::Success)->show();
            this->onClose(this);
        })
        .parent(buttonMenu)
        .store(sendButton);

    auto& am = AdminManager::get();
    if (am.authorized()) {
        auto& role = am.getRole();
        if (role.editFeaturedLevels) {
            Build<ButtonSprite>::create("Feature", "bigFont.fnt", "GJ_button_02.png", 0.9f)
                .scale(0.75f)
                .intoMenuItem([this] {
                    this->save();
                })
                .parent(buttonMenu)
                .store(featureButton);
        }
    }

    buttonMenu->updateLayout();

    return true;
}

void EditFeaturedLevelPopup::save() {
    //int levelId = util::format::parse<int>(idInput->getString()).value_or(0);
    auto& am = AdminManager::get();
    if (am.authorized()) {
        auto& role = am.getRole();
        if (role.editFeaturedLevels) {
            int levelId = this->level->m_levelID;

            if (levelId == 0) {
                ErrorQueues::get().warn("Invalid level ID");
                return;
            }

            auto req = WebRequestManager::get().setFeaturedLevel(levelId, currIdx, level->m_levelName, level->m_creatorName, util::gd::calcLevelDifficulty(level));
            reqListener.bind(this, &EditFeaturedLevelPopup::onRequestComplete);
            reqListener.setFilter(std::move(req));
        } else {
            ErrorQueues::get().warn("You don't have permission to do this");
            this->onClose(this);
        }
    }
}

void EditFeaturedLevelPopup::createDiffButton() {
    if (curDiffButton) curDiffButton->removeFromParent();

    auto rlayout = util::ui::getPopupLayoutAnchored(m_size);

    auto spr = DailyManager::get().createRatingSprite(currIdx);
    spr->setScale(1.25);

    Build<CCMenuItemSpriteExtra>::create(spr, this, menu_selector(EditFeaturedLevelPopup::onDiffClick))
        .store(curDiffButton)
        .parent(menu);
}

void EditFeaturedLevelPopup::onDiffClick(CCObject* sender) {
    this->currIdx.increment();

    this->createDiffButton();
}

int EditFeaturedLevelPopup::getDifficulty() {
    return util::gd::calcLevelDifficulty(level);
}

void EditFeaturedLevelPopup::onRequestComplete(typename WebRequestManager::Event* event) {
    if (!event || !event->getValue()) return;

    auto result = std::move(*event->getValue());

    if (result.isOk()) {
        ErrorQueues::get().success("Successfully updated the level");
    } else {
        auto err = result.unwrapErr();
        ErrorQueues::get().error(fmt::format("Failed to update the level.\n\nReason: <cy>{}</c>", util::format::webError(err)));
    }

    this->onClose(this);
}

EditFeaturedLevelPopup* EditFeaturedLevelPopup::create(GJGameLevel* level) {
    auto ret = new EditFeaturedLevelPopup();
    if (ret->initAnchored(POPUP_WIDTH, POPUP_HEIGHT, level)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}
