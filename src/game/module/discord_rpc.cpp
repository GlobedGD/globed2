#include "discord_rpc.hpp"

#include <hooks/gjbasegamelayer.hpp>
#include <Geode/loader/Dispatch.hpp>

using namespace geode::prelude;

void DiscordRpcModule::selUpdate(float dt) {
    this->counter += dt;

    if (this->counter > 10.f) {
        this->counter = 0.f;

        this->postDRPCEvent();
    }
}

void DiscordRpcModule::postInitActions() {
    this->counter = 0.f;
    this->postDRPCEvent();
}

void DiscordRpcModule::postDRPCEvent() {
    auto m_level = gameLayer->m_level;

    // taken from drpc
    bool isRobTopLevel = (
        (m_level->m_levelID.value() < 128 && m_level->m_levelID.value() != 0) ||
            m_level->m_levelID.value() == 3001 || m_level->m_levelID.value() < 5003 ||
            m_level->m_levelID.value() > 5000
    ) && m_level->m_creatorName.empty();

    auto state = fmt::format("{} by {}", std::string(m_level->m_levelName), (isRobTopLevel) ? "RobTopGames" : std::string(m_level->m_creatorName));

    using SetRPCEvent = geode::DispatchEvent<bool>;
    SetRPCEvent("techstudent10.discord_rich_presence/set_default_rpc_enabled", false).post();

    using UpdateRPCEvent = geode::DispatchEvent<std::string>;
    auto json = matjson::Value(matjson::Object({
        {"modID", ""_spr},
        {"details", "Playing on Globed!"},
        {"state", state},
        {"smallImageKey", ""},
        {"smallImageText", ""},
        {"useTime", true},
        {"shouldResetTime", false},
        {"largeImageKey", "https://raw.githubusercontent.com/dankmeme01/globed2/main/logo.png"},
        {"largeImageText", ""},
        {"joinSecret", std::to_string(m_level->m_levelID.value())},
        {"partyMax", gameLayer->m_fields->players.size() + 1}
    })).dump();
    UpdateRPCEvent("techstudent10.discord_rich_presence/update_rpc", json).post();
}

void DiscordRpcModule::onQuit() {
    // report back to the RPC mod that it can takeover again
    using SetRPCEvent = geode::DispatchEvent<bool>;
    SetRPCEvent("techstudent10.discord_rich_presence/set_default_rpc_enabled", true).post();
}
