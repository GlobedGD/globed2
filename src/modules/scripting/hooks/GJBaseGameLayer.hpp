#pragma once
#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <globed/config.hpp>
#include <globed/core/data/Messages.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <modules/scripting/ScriptingModule.hpp>
#include <modules/scripting/objects/ListenEventObject.hpp>
#include "Common.hpp"

namespace globed {

struct pairhash {
public:
    template <typename T, typename U>
    size_t operator()(const std::pair<T, U>& x) const {
        auto ha = std::hash<T>()(x.first);
        auto hb = std::hash<U>()(x.second);

        return ha ^ (hb + 0x9e3779b9 + (ha << 6) + (ha >> 2));
    }
};

struct GLOBED_NOVTABLE GLOBED_DLL SCBaseGameLayer : geode::Modify<SCBaseGameLayer, GJBaseGameLayer> {
    struct Fields {
        std::optional<MessageListener<msg::LevelDataMessage>> m_listener;
        std::optional<MessageListener<msg::ScriptLogsMessage>> m_logsListener;
        std::vector<std::string> m_logBuffer;

        // std::unordered_map<uint16_t, std::vector<int>> m_customListeners;
        std::unordered_set<std::pair<int, int>, pairhash> m_customFollowers;
        std::unordered_map<int, cocos2d::CCPoint> m_lastPlayerPositions;
    };

    static SCBaseGameLayer* get(GJBaseGameLayer* base = nullptr);

    void postInit(const std::vector<EmbeddedScript>& scripts);
    void handleEvent(const Event& event);

    void addEventListener(const ListenEventPayload& obj);
    void sendLogRequest(float);
    std::vector<std::string>& getLogs();

    void customMoveBy(int group, double dx, double dy);
    void customMoveTo(int group, int center, double x, double y);
    void customMoveDirection(
        int group,
        int targetPosGroup,
        int centerGroup,
        float distance,
        float time,
        bool silent,
        bool onlyX,
        bool onlyY,
        bool dynamic
    );
    void customFollowPlayer(int id, int group, bool enable);

    void unfollowAllForPlayer(int id);
    void processCustomFollowActions(float);
};

}