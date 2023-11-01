/*
* To add a new packet, your chores are:
* 1. create a header file in client/ or server/
* 2. inherit Packet and add GLOBED_PACKET(id, encrypt), encrypt should be true for packets that are important.
* 3. add GLOBED_ENCODE and GLOBED_DECODE (you can replcae either with the _UNIMPL version if not meant to be used)
* 4. For client packets, you may also choose to add a ::create(...) function and/or a constructor
* 5. For server packets, in `all.cpp` add the packet to the switch as PACKET(id, cls), make sure that IDs are sorted properly.

*
* Make sure packet ID is in the right category, 1xxxx - upstream, 2xxxx - downstream
* x0xxx - connection-related (encryption, keepalive, ping, handshake)
* x1xxx - level-related (player data, chat, account data)
* x2xxx - misc
*/

#pragma once

#include "packet.hpp"

#include "client/ping.hpp"
#include "client/crypto_handshake_start.hpp"

#include "server/ping_response.hpp"
#include "server/crypto_handshake_response.hpp"

// Matches a packet by packet ID, returns nullptr if not found. Otherwise returns an Packet* with uninitialized data
std::shared_ptr<Packet> matchPacket(packetid_t packetId);