#include "ConnectionLogger.hpp"
#include <arc/future/Select.hpp>
#include <asp/time/SystemTime.hpp>
#include <asp/fs.hpp>

using namespace geode::prelude;
using namespace asp::time;
using namespace arc;

namespace globed {

ConnectionLogger::ConnectionLogger() {}

ConnectionLogger::~ConnectionLogger() {
    // do not wait for task to finish
    if (m_handle) {
        m_handle->abort();
    }
}

void ConnectionLogger::resetInternal() {
    // flush log
    if (!m_curLog.empty()) {
        log::info("Dumping all connection logs to {}", m_curPath);

        if (auto err = asp::fs::write(m_curPath / "log.txt", m_curLog).err()) {
            log::error("ConnectionLogger: failed to write log file: {}", err);
        }
    }

    m_curPath = m_basePath / SystemTime::now().format("{:%Y-%m-%dT%H-%M-%S}");
    m_curLog.clear();
    m_startTime = Instant::now();

    log::info("Setup logger at {}", m_curPath);
}

Future<> ConnectionLogger::setup(std::filesystem::path path) {
    m_basePath = std::move(path);

    auto [pTx, pRx] = arc::mpsc::channel<PacketLog>(128);
    auto [tTx, tRx] = arc::mpsc::channel<std::string>(256);
    m_packetLogTx = std::move(pTx);
    m_textLogTx = std::move(tTx);

    m_handle = arc::spawn(
        [this, pRx = std::move(pRx), tRx = std::move(tRx)](this auto self) -> Future<> {
        while (true) {
            co_await arc::select(
                arc::selectee(
                    m_reset.notified(),
                    [&] {
                        // drain receiver and reset folder
                        pRx.drain();
                        tRx.drain();
                        this->resetInternal();
                    }
                ),

                arc::selectee(
                    pRx.recv(),
                    [&](auto result) -> arc::Future<> {
                        if (!result) co_return;
                        co_await this->doLog(std::move(result).unwrap());
                    }
                ),

                arc::selectee(
                    tRx.recv(),
                    [&](auto result) {
                        if (!result) return;
                        auto msg = std::move(result).unwrap();
                        m_curLog += fmt::format("[{:.6f}] {}\n", m_startTime.elapsed().seconds<double>(), msg);
                    }
                )
            );
        }

        co_return;
    }());

    this->resetInternal();

    co_return;
}

void ConnectionLogger::sendPacketLog(std::vector<uint8_t> data, bool up) {
    PacketLog log {
        .when = Instant::now(),
        .data = std::move(data),
        .up = up,
    };

    if (m_packetLogTx) {
        (void) m_packetLogTx->trySend(std::move(log));
    }
}

void ConnectionLogger::sendTextLog(std::string msg) {
    if (m_textLogTx) {
        (void) m_textLogTx->trySend(std::move(msg));
    }
}

void ConnectionLogger::reset() {
    m_reset.notifyOne();
}

Future<> ConnectionLogger::doLog(PacketLog log) {
    co_await arc::spawnBlocking<void>([this, log = std::move(log)] {
        if (!asp::fs::exists(m_curPath)) {
            if (auto err = asp::fs::createDirAll(m_curPath).err()) {
                log::error("ConnectionLogger: failed to create log directory: {}", err->message());
                return;
            }
        }

        auto direction = log.up ? "up" : "down";
        auto filename = fmt::format("{:.3}s_{}.bin", m_startTime.elapsed().seconds<double>(), direction);
        auto path = m_curPath / filename;

        if (auto err = asp::fs::write(path, log.data).err()) {
            log::error("ConnectionLogger: failed to write packet log file: {}", err);
        }
    });
}

}