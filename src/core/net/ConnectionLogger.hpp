#pragma once

#include <arc/task/Task.hpp>
#include <arc/sync/mpsc.hpp>
#include <arc/runtime/Runtime.hpp>
#include <filesystem>

namespace globed {

struct PacketLog {
    asp::time::Instant when;
    std::vector<uint8_t> data;
    bool up;
};

class ConnectionLogger {
public:
    ConnectionLogger();
    ~ConnectionLogger();

    arc::Future<> setup(std::filesystem::path path);
    void reset();

    void sendPacketLog(std::vector<uint8_t> data, bool up);
    void sendTextLog(std::string msg);

private:
    std::optional<arc::TaskHandle<void>> m_handle;
    std::optional<arc::mpsc::Sender<PacketLog>> m_packetLogTx;
    std::optional<arc::mpsc::Sender<std::string>> m_textLogTx;
    std::filesystem::path m_basePath;
    std::filesystem::path m_curPath;
    asp::time::Instant m_startTime;
    std::string m_curLog;
    arc::Notify m_reset;

    void resetInternal();
    arc::Future<> doLog(PacketLog log);
};

}