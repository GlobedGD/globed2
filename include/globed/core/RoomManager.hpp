#pragma once

#include "../util/singleton.hpp"
#include "../core/data/RoomSettings.hpp"
#include "../core/data/RoomTeam.hpp"
#include "../core/SessionId.hpp"
#include <asp/sync/Mutex.hpp>

namespace globed {

/// Class that manages client's current room state.
/// It also manages joining and leaving levels.
/// All functions in this class are safe to call from any thread unless noted otherwise.
class GLOBED_DLL RoomManager : public SingletonLeakBase<RoomManager> {
public:
    // helper funcs

    static std::optional<SessionId> getEditorCollabId(GJGameLevel* level);
    SessionId makeSessionId(int levelId);
    std::optional<uint8_t> pickServerId();

    // joining / leaving levels

    void joinLevel(int levelId, int author, bool platformer, bool editorCollab);
    void joinLevel(GJGameLevel* level);

    void leaveLevel();

    /// Resets various state, called on disconnect
    void reset();

    // getters for room stuff

    uint32_t getRoomId();
    /// Returns account ID of the person who currently manages this room
    int32_t getRoomOwner();
    std::string getRoomName();
    RoomSettings getSettings();
    uint32_t getPasscode();
    SessionId getPinnedLevel();
    SessionId getCurrentWarpLevel();

    bool isInGlobal();
    bool isInRoom();
    bool isInFollowerRoom();
    bool isOwner();

    // team getters

    uint16_t getCurrentTeamId();
    std::optional<RoomTeam> getCurrentTeam();
    std::optional<RoomTeam> getTeam(uint16_t id);
    std::optional<uint16_t> getTeamIdForPlayer(int player);

    // misc

    void setAttemptedPasscode(uint32_t code);

private:
    friend class SingletonLeakBase<RoomManager>;
    RoomManager();
    ~RoomManager() = default;

    struct State {
        uint32_t m_roomId = 0;
        uint32_t m_passcode = 0;
        int m_roomOwner = 0;
        SessionId m_pinnedLevel;
        SessionId m_currentWarpLevel;
        std::string m_roomName;
        RoomSettings m_settings{};

        uint16_t m_teamId = 0;
        std::unordered_map<uint16_t, std::vector<int>> m_teamMembers;
        std::vector<RoomTeam> m_teams;

        void resetValues();
    };

    asp::Mutex<State> m_state;
};

}