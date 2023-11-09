/*
* To add a new packet, your chores are:
* 1. find the appropriate category that suits it and make a proper packet ID
* client/ - 1xxxx, server/ - 2xxxx
* connection.hpp - x0xxx - connection-related (encryption, keepalive, ping, handshake)
* game.hpp - x1xxx - level-related (player data, chat, account data)
* misc.hpp - x2xxx - misc
*
* 2. in your class, inherit Packet and add GLOBED_PACKET(id, encrypt), encrypt should be true for packets that are important.
* 3. add GLOBED_ENCODE and GLOBED_DECODE (you can replcae either with the _UNIMPL version if not meant to be used)
* 4. For client packets, you may also choose to add a ::create(...) function and/or a constructor
* 5. For server packets, in `all.cpp` add the packet to the switch as PACKET(id, cls).
*/

#pragma once
#include <defs.hpp>

#include "packet.hpp"

#include "client/connection.hpp"
#include "client/game.hpp"

#include "server/connection.hpp"
#include "server/game.hpp"

// Matches a packet by packet ID, returns nullptr if not found. Otherwise returns an Packet* with uninitialized data
std::shared_ptr<Packet> matchPacket(packetid_t packetId);