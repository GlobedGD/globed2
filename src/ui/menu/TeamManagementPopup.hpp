#pragma once

#include <globed/core/net/MessageListener.hpp>
#include <globed/core/data/Messages.hpp>
#include <ui/BasePopup.hpp>

#include <cue/ListNode.hpp>
#include <cue/LoadingCircle.hpp>

namespace globed {

class TeamManagementPopup : public BasePopup<TeamManagementPopup> {
public:
    static const cocos2d::CCSize POPUP_SIZE;

protected:
    friend class TeamCell;
    cue::ListNode* m_list;
    cue::LoadingCircle* m_loadingCircle = nullptr;
    std::optional<MessageListener<msg::RoomStateMessage>> m_stateListener;
    std::optional<MessageListener<msg::TeamCreationResultMessage>> m_creationListener;

    bool setup();
    void startLoading();
    void onLoaded(const std::vector<RoomTeam>& teams);
    void onTeamCreated(bool success, uint16_t teamCount);

    void stopLoad();
    void addPlusButton();

    void createTeam();
    void deleteTeam(uint16_t teamId);
    void updateTeamColor(uint16_t teamId, cocos2d::ccColor4B color);
};

}