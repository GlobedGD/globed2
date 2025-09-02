#include "RoomUserControlsPopup.hpp"
#include <globed/core/PopupManager.hpp>
#include <core/net/NetworkManagerImpl.hpp>
#include <ui/menu/TeamManagementPopup.hpp>

#include <UIBuilder.hpp>
#include <cue/Util.hpp>

using namespace geode::prelude;

namespace globed {

const CCSize RoomUserControlsPopup::POPUP_SIZE{ 200.f, 140.f };

bool RoomUserControlsPopup::setup(int id, std::string_view username) {
    m_username = username;
    m_accountId = id;
    this->setTitle("User actions");

    m_menu = Build<CCMenu>::create()
        .contentSize(160.f, 60.f)
        .anchorPoint(0.5f, 0.5f)
        .ignoreAnchorPointForPos(false)
        .pos(this->fromCenter(0.f, -10.f))
        .layout(RowLayout::create()->setAutoScale(false))
        .parent(m_mainLayer)
        .collect();

    this->remakeButtons();

    return true;
}

void RoomUserControlsPopup::remakeButtons() {
    m_menu->removeAllChildren();

    CCSize btnSize { 28.f, 28.f };

    Build<CCSprite>::create("button-admin-ban.png"_spr)
        .with([&](auto spr) { cue::rescaleToMatch(spr, btnSize); })
        .intoMenuItem([this] {
            globed::quickPopup(
                "Ban user",
                fmt::format("Are you sure you want to <cr>ban</c> <cy>{}</c> from your room? They will be unable to join until you create a new room.", m_username),
                "Cancel",
                "Ban",
                [this](auto, bool yup) {
                    if (!yup) return;

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
            globed::quickPopup(
                "Kick user",
                fmt::format("Are you sure you want to <cr>kick</c> <cy>{}</c> from your room? They will be able to join again.", m_username),
                "Cancel",
                "Kick",
                [this](auto, bool yup) {
                    if (!yup) return;

                    this->onClose(nullptr);

                    NetworkManagerImpl::get().sendRoomOwnerAction(RoomOwnerActionType::KICK_USER, m_accountId);
                }
            );
        })
        .scaleMult(1.1f)
        .parent(m_menu);

    Build(EditorButtonSprite::createWithSprite("icon-person.png"_spr, 1.f, EditorBaseColor::Cyan))
        .with([&](auto spr) { cue::rescaleToMatch(spr, btnSize); })
        .intoMenuItem([this] {
            TeamManagementPopup::create(m_accountId)->show();
        })
        .scaleMult(1.1f)
        .parent(m_menu);

    m_menu->updateLayout();
}

}