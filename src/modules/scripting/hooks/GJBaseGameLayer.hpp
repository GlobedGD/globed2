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

struct GLOBED_NOVTABLE GLOBED_DLL SCBaseGameLayer : geode::Modify<SCBaseGameLayer, GJBaseGameLayer> {
    struct Fields {
        std::optional<MessageListener<msg::LevelDataMessage>> m_listener;
        std::optional<MessageListener<msg::ScriptLogsMessage>> m_logsListener;
        std::vector<std::string> m_logBuffer;

        // std::unordered_map<uint16_t, std::vector<int>> m_customListeners;
    };

    static SCBaseGameLayer* get(GJBaseGameLayer* base = nullptr);

    void postInit(const std::vector<EmbeddedScript>& scripts);
    void handleEvent(const Event& event);

    void addEventListener(const ListenEventPayload& obj);
    void sendLogRequest(float);
    std::vector<std::string>& getLogs();
};

}