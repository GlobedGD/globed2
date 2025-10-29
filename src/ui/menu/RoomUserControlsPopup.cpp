#include "RoomUserControlsPopup.hpp"
#include <globed/core/PopupManager.hpp>
#include <globed/core/RoomManager.hpp>
#include <core/net/NetworkManagerImpl.hpp>
#include <ui/menu/TeamManagementPopup.hpp>
#include <ui/admin/ModUserPopup.hpp>

#include <UIBuilder.hpp>
#include <cue/Util.hpp>

using namespace geode::prelude;

namespace globed {

const CCSize RoomUserControlsPopup::POPUP_SIZE{ 170.f, 90.f };

bool RoomUserControlsPopup::setup(int id, std::string_view username) {
    m_username = username;
    m_accountId = id;
    this->setTitle("User actions");

    m_menu = Build<CCMenu>::create()
        .contentSize(160.f, 60.f)
        .anchorPoint(0.5f, 0.5f)
        .ignoreAnchorPointForPos(false)
        .pos(this->fromCenter(0.f, -14.f))
        .layout(RowLayout::create()->setAutoScale(false))
        .parent(m_mainLayer)
        .collect();

    this->remakeButtons();

    return true;
}

void RoomUserControlsPopup::remakeButtons() {
    m_menu->removeAllChildren();

    CCSize btnSize { 36.f, 36.f };

    auto& rm = RoomManager::get();

    Build<CCSprite>::create("button-admin-ban.png"_spr)
        .with([&](auto spr) { cue::rescaleToMatch(spr, btnSize); })
        .intoMenuItem([this] {
            globed::confirmPopup(
                "Ban user",
                fmt::format("Are you sure you want to <cr>ban</c> <cy>{}</c> from your room? They will be unable to join until you create a new room.", m_username),
                "Cancel",
                "Ban",
                [this](auto) {
                    this->onClose(nullptr);

                    NetworkManagerImpl::get().sendRoomOwnerAction(RoomOwnerActionType::BAN_USER, m_accountId);
                }
            );
        })
        .scaleMult(1.1f)
        .parent(m_menu);

    Build<CCSprite>::create("button-admin-kick.png"_spr)
        .with([&](auto spr) { cue::rescaleToMatch(spr, btnSize); })
        .intoMenuItem([this] {
            globed::confirmPopup(
                "Kick user",
                fmt::format("Are you sure you want to <cr>kick</c> <cy>{}</c> from your room? They will be able to join again.", m_username),
                "Cancel",
                "Kick",
                [this](auto) {
                    this->onClose(nullptr);

                    NetworkManagerImpl::get().sendRoomOwnerAction(RoomOwnerActionType::KICK_USER, m_accountId);
                }
            );
        })
        .scaleMult(1.1f)
        .parent(m_menu);

    if (rm.getSettings().teams) {
        Build(EditorButtonSprite::createWithSprite("icon-person.png"_spr, 1.f, EditorBaseColor::Cyan))
            .with([&](auto spr) { cue::rescaleToMatch(spr, btnSize); })
            .intoMenuItem([this] {
                TeamManagementPopup::create(m_accountId)->show();
            })
            .scaleMult(1.1f)
            .parent(m_menu);
    }


    // TODO change icon?
    if (NetworkManagerImpl::get().isAuthorizedModerator()) {
        Build<CCSprite>::createSpriteName("GJ_reportBtn_001.png")
            .with([&](auto spr) { cue::rescaleToMatch(spr, btnSize); })
            .intoMenuItem([this] {
                ModUserPopup::create(m_accountId)->show();
            })
            .scaleMult(1.1f)
            .parent(m_menu);
    }

    m_menu->updateLayout();
}

}