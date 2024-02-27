#pragma once
#include "game_socket.hpp"

#include <functional>
#include <unordered_map>
#include <thread>

#include <managers/game_server.hpp>
#include <managers/central_server.hpp>
#include <util/sync.hpp>
#include <util/time.hpp>
#include <util/format.hpp>

using namespace util::sync;

enum class NetworkThreadTask {
    PingServers
};

template <typename T>
concept HasPacketID = requires { T::PACKET_ID; };

constexpr int ROLE_USER = 0;
constexpr int ROLE_HELPER = 1;
constexpr int ROLE_MOD = 2;
constexpr int ROLE_ADMIN = 100;

// This class is fully thread safe..? hell do i know..
class NetworkManager : public SingletonBase<NetworkManager> {
protected:
    friend class SingletonBase;
    NetworkManager();
    ~NetworkManager();

public:
    using PacketCallback = std::function<void(std::shared_ptr<Packet>)>;

    template <HasPacketID Pty>
    using PacketCallbackSpecific = std::function<void(std::shared_ptr<Pty>)>;

    static constexpr uint16_t PROTOCOL_VERSION = 4;
    static constexpr util::data::byte SERVER_MAGIC[10] = {0xda, 0xee, 'g', 'l', 'o', 'b', 'e', 'd', 0xda, 0xee};

    AtomicU32 connectedTps; // if `authenticated() == true`, this is the TPS of the current server, otherwise undefined.

    // Connect to a server
    Result<> connect(const std::string_view addr, unsigned short port, const std::string_view serverId, bool standalone = false);
    // Safer version of `connect`, sets the active game server in `GameServerManager` on success, doesn't throw on exception on error
    Result<> connectWithView(const GameServer& gsview);
    // Is similar to `connectWithView` (does not throw exceptions) but is made specifically for standalone servers.
    // Grabs the address from the first server in `GameServerManager`
    Result<> connectStandalone();

    // Disconnect from a server. Does nothing if not connected
    void disconnect(bool quiet = false, bool noclear = false);

    // Call `disconnect` and show an error popup with a message
    void disconnectWithMessage(const std::string_view message, bool quiet = true);

    // Sends a packet to the currently established connection. Throws if disconnected.
    void send(std::shared_ptr<Packet> packet);

    // Adds a packet listener and calls your callback function when a packet with `id` is received.
    // If there already was a callback with this packet ID, it gets replaced.
    // All callbacks are ran in the main (GD) thread.
    void addListener(packetid_t id, PacketCallback&& callback);

    // Same as addListener(packetid_t, PacketCallback) but hacky syntax xd
    template <HasPacketID Pty>
    void addListener(PacketCallbackSpecific<Pty>&& callback) {
        this->addListener(Pty::PACKET_ID, [callback](std::shared_ptr<Packet> pkt) {
            callback(std::static_pointer_cast<Pty>(pkt));
        });
    }

    // Removes a listener by packet ID.
    void removeListener(packetid_t id);

    // Same as removeListener(packetid_t) but hacky syntax once again
    template <HasPacketID T>
    void removeListener() {
        this->removeListener(T::PACKET_ID);
    }

    // Same as `removeListener<T>` and `suppressUnhandledFor<T>(duration)` combined
    template <HasPacketID T, typename Rep, typename Period>
    void removeListener(util::time::duration<Rep, Period> duration) {
        this->removeListener<T>();
        this->suppressUnhandledFor<T>(duration);
    }

    template <HasPacketID T>
    void removeListenerDelayed() {
        Loader::get()->queueInMainThread([this] {
            this->removeListener<T>();
        });
    }

    template <HasPacketID T, typename Rep, typename Period>
    void removeListenerDelayed(util::time::duration<Rep, Period> duration) {
        Loader::get()->queueInMainThread([this, duration = duration] {
            this->removeListener<T>(duration);
        });
    }

    // Removes all listeners.
    void removeAllListeners();

    // queues task for pinging servers
    void taskPingServers();

    // Returns true if ANY connection has been made with a server. The handshake might not have been done at this point.
    bool connected();

    // Returns true ONLY if we are connected to a server and the crypto handshake has finished. We might not have logged in yet.
    bool handshaken();

    // Returns true if we have fully authenticated and are ready to rock.
    bool established();

    // Returns true if we are connected to a server and authorized as an admin
    bool isAuthorizedAdmin();

    void clearAdminStatus();

    int getAdminRole();

    // Returns true if we are connected to a standalone game server, not tied to any central server.
    bool standalone();

    void suspend();
    void resume();

    template <HasPacketID Packet, typename Rep, typename Period>
    void suppressUnhandledFor(util::time::duration<Rep, Period> duration) {
        auto endPoint = util::time::systemNow() + duration;
        (*suppressed.lock())[Packet::PACKET_ID] = endPoint;
    }

private:
    static constexpr chrono::seconds KEEPALIVE_INTERVAL = chrono::seconds(5);
    static constexpr chrono::seconds TCP_KEEPALIVE_INTERVAL = chrono::seconds(60);
    static constexpr chrono::seconds DISCONNECT_AFTER = chrono::seconds(15);

    GameSocket gameSocket;

    SmartMessageQueue<std::shared_ptr<Packet>> packetQueue;
    SmartMessageQueue<NetworkThreadTask> taskQueue;

    WrappingMutex<std::unordered_map<packetid_t, PacketCallback>> listeners;
    WrappingMutex<std::unordered_map<packetid_t, util::time::system_time_point>> suppressed;

    // threads

    void threadMainFunc();
    void threadRecvFunc();

    SmartThread<NetworkManager*> threadMain;
    SmartThread<NetworkManager*> threadRecv;

    // misc

    AtomicBool _handshaken = false;
    AtomicBool _loggedin = false;
    AtomicBool _adminAuthorized = false;
    AtomicI32 _adminRole = ROLE_USER;
    AtomicBool _connectingStandalone = false;
    AtomicBool _suspended = false;
    AtomicBool _deferredConnect = false;
    uint32_t secretKey = 0;

    std::string _deferredAddr, _deferredServerId;
    unsigned short _deferredPort;

    util::time::time_point lastKeepalive;
    util::time::time_point lastTcpKeepalive;
    util::time::time_point lastReceivedPacket;

    void handlePingResponse(std::shared_ptr<Packet> packet);
    void maybeSendKeepalive();
    void maybeDisconnectIfDead();

    // Builtin listeners have priority above the others.
    WrappingMutex<std::unordered_map<packetid_t, PacketCallback>> builtinListeners;

    void addBuiltinListener(packetid_t id, PacketCallback&& callback);

    // Callbacks are NOT ran from the main thread.
    template <HasPacketID Pty>
    void addBuiltinListener(PacketCallbackSpecific<Pty>&& callback) {
        this->addBuiltinListener(Pty::PACKET_ID, [callback = std::move(callback)](std::shared_ptr<Packet> pkt) {
            callback(std::static_pointer_cast<Pty>(pkt));
        });
    }
};