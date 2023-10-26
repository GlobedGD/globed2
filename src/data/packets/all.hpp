/*
* To add a new packet, your chores are:
* 1. create a header file in client/ or server/
* 2. copy the basic structure from ping_packet.hpp, inherit Packet and add GLOBED_PACKET(id, encrypt)
* 3. override the decode and encode functions (put GLOBED_DECODE_UNIMPL or GLOBED_ENCODE_UNIMPL if the function isn't supposed to be used on the client)
* 4. in `all.cpp` add the packet to the switch as PACKET(id, cls), make sure that IDs are sorted properly
* 5. For client packets, you may also choose to add a ::create(...) function and/or a constructor
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