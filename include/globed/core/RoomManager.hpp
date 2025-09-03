#pragma once

#include <globed/util/singleton.hpp>
#include <globed/core/data/RoomSettings.hpp>
#include <globed/core/data/RoomTeam.hpp>

namespace globed {

/// Class that manages client's current room state.
/// It also manages joining and leaving levels.
class RoomManager : public SingletonLeakBase<RoomManager> {
public:
    void joinLevel(int levelId, bool platformer);
    void leaveLevel();

    bool isInGlobal();
    bool isInFollowerRoom();
    bool isOwner();
    uint32_t getRoomId();
    int getRoomOwner();
    std::optional<uint8_t> pickServerId();
    RoomSettings& getSettings();
    void setAttemptedPasscode(uint32_t code);
    uint32_t getPasscode();

    uint16_t getCurrentTeamId();
    std::optional<RoomTeam> getCurrentTeam();
    std::optional<RoomTeam> getTeam(uint16_t id);
    std::optional<uint16_t> getTeamIdForPlayer(int player);

private:
    friend class SingletonLeakBase;
    RoomManager();
    ~RoomManager() = default;

    uint32_t m_roomId = 0;
    uint32_t m_passcode = 0;
    int m_roomOwner = 0;
    std::string m_roomName;
    RoomSettings m_settings{};

    uint16_t m_teamId = 0;
    std::unordered_map<uint16_t, std::vector<int>> m_teamMembers;
    std::vector<RoomTeam> m_teams;

    void resetValues();

};

}