#pragma once
#include "game_socket.hpp"
#include <defs.hpp>
#include <util/sync.hpp>

#include <functional>
#include <unordered_map>
#include <thread>

using namespace util::sync;

enum class NetworkThreadTask {
    PingServers
};

class NetworkManager {
public:
    using PacketCallback = std::function<void(std::shared_ptr<Packet>)>;

    GLOBED_SINGLETON(NetworkManager)
    NetworkManager();
    ~NetworkManager();

    // Connect to a server
    void connect(const std::string& addr, unsigned short port);

    // Disconnect from a server. Does nothing if not connected
    void disconnect();

    // Sends a packet to the currently established connection. Throws if disconnected.
    void send(Packet* packet);

    // Adds a packet listener and calls your callback function when a packet with `id` is received.
    // If there already was a callback with this packet ID, it gets replaced.
    // All callbacks are ran in the main (GD) thread.
    void addListener(packetid_t id, PacketCallback callback);

    // Removes a listener by packet ID.
    void removeListener(packetid_t id);

    // Removes all listeners.
    void removeAllListeners();

    // queues task for pinging servers
    void taskPingServers();

    // Returns true if ANY connection has been made with a server. The handshake might not have been done at this point.
    bool connected();

    // Returns true ONLY if we are connected to a server and the crypto handshake has finished.
    bool established();

private:
    GameSocket socket, pingSocket;

    SmartMessageQueue<Packet*> packetQueue;
    SmartMessageQueue<NetworkThreadTask> taskQueue;

    WrappingMutex<std::unordered_map<packetid_t, PacketCallback>> listeners;

    // threads

    void threadMainFunc();
    void threadRecvFunc();
    void threadTasksFunc();
    void threadPingRecvFunc();

    std::thread threadMain;
    std::thread threadRecv;
    std::thread threadTasks;
    std::thread threadPingRecv;

    std::atomic_bool _running = true;

    std::atomic_bool _established = false;
};