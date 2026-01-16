#pragma once
#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <globed/config.hpp>
#include <globed/core/data/Messages.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <modules/scripting/ScriptingModule.hpp>
#include <modules/scripting/data/EmbeddedScript.hpp>
#include <modules/scripting/objects/ListenEventObject.hpp>

#include <asp/time/SystemTime.hpp>

namespace globed {

struct pairhash {
public:
    template <typename T, typename U> size_t operator()(const std::pair<T, U> &x) const
    {
        auto ha = std::hash<T>()(x.first);
        auto hb = std::hash<U>()(x.second);

        return ha ^ (hb + 0x9e3779b9 + (ha << 6) + (ha >> 2));
    }
};

struct CustomFollowAction {
    int m_playerId;
    int m_groupId;
    int m_centerGroupId = 0;
    bool m_pos, m_rot;

    bool operator==(const CustomFollowAction &) const = default;
};

struct CustomFollowedData {
    cocos2d::CCPoint pos;
    float rot;
};

struct GLOBED_MODIFY_ATTR SCBaseGameLayer : geode::Modify<SCBaseGameLayer, GJBaseGameLayer> {
    struct Fields {
        std::optional<MessageListener<msg::LevelDataMessage>> m_listener;
        std::optional<MessageListener<msg::ScriptLogsMessage>> m_logsListener;
        std::vector<std::string> m_logBuffer;
        std::deque<std::pair<asp::time::SystemTime, float>> m_memLimitBuffer;

        // std::unordered_map<uint16_t, std::vector<int>> m_customListeners;
        std::vector<CustomFollowAction> m_followActions;
        std::unordered_map<int, CustomFollowedData> m_lastPlayerPositions;
        int m_localId = 0;
        bool m_hasScripts = false;
        bool m_scriptsSent = false;
    };

    static SCBaseGameLayer *get(GJBaseGameLayer *base = nullptr);

    void postInit(const std::vector<EmbeddedScript> &scripts);
    void handleEvent(const InEvent &event);

    void addEventListener(const ListenEventPayload &obj);
    void sendLogRequest(float);
    std::vector<std::string> &getLogs();
    std::deque<std::pair<asp::time::SystemTime, float>> &getMemLimitBuffer();

    void customMoveBy(int group, double dx, double dy);
    void customMoveTo(int group, int center, double x, double y);
    void customMoveDirection(int group, int targetPosGroup, int centerGroup, float distance, float time, bool silent,
                             bool onlyX, bool onlyY, bool dynamic);
    void customRotateBy(int group, int center, double theta);

    void customFollowPlayerMov(int player, int group, bool enable);
    void customFollowPlayerRot(int player, int group, int center, bool enable);
    void removeCustomFollow(int player, int group);

    CustomFollowAction &insertCustomFollow(int player, int group);
    void disableCustomFollow(int player, int group, bool disableMov, bool disableRot);
    size_t getCustomFollowIndex(int player, int group);
    CustomFollowedData positionForPlayer(int player);

    void unfollowAllForPlayer(int id);
    void processCustomFollowActions(float);

    void rotateObjects(cocos2d::CCArray *p0, float p1, cocos2d::CCPoint p2, cocos2d::CCPoint p3, bool p4, bool p5)
    {
        geode::log::debug("rotateObjects({}, {}, {}, {}, {}, {})", p0, p1, p2, p3, p4, p5);
        GJBaseGameLayer::rotateObjects(p0, p1, p2, p3, p4, p5);
    }
};

} // namespace globed
