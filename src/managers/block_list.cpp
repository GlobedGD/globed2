#include "block_list.hpp"

#include <data/bytebuffer.hpp>
#include <util/crypto.hpp>

BlockListManager::BlockListManager() {
    try {
        auto result = this->load();
        if (result.isErr()) {
            log::warn("Faile to load blocklist: {}", result.unwrapErr());
            _bl.clear();
            _wl.clear();
        }
    } catch (const std::exception& e) {
        _bl.clear();
        _wl.clear();
    }
}

void BlockListManager::save() {
    // guys laugh at me all you want but in c++ this is easier and faster than using comma as a separator
    ByteBuffer bb;
    bb.writeU32(_bl.size());
    for (int elem : _bl) {
        bb.writeI32(elem);
    }

    bb.writeU32(_wl.size());
    for (int elem : _wl) {
        bb.writeI32(elem);
    }

    auto data = util::crypto::base64Encode(bb.data());

    Mod::get()->setSavedValue(SETTING_KEY, data);
}

Result<> BlockListManager::load() {
    _bl.clear();
    _wl.clear();

    auto val = Mod::get()->getSavedValue<std::string>(SETTING_KEY);
    if (val.empty()) return Err("no blocklist found");

    util::data::bytevector data;

    try {
        data = util::crypto::base64Decode(val);
    } catch (const std::exception& e) {
        return Err(e.what());
    }

    if (data.empty()) return Err("base64 string was empty");

    ByteBuffer bb(std::move(data));
    auto blocklistR = bb.readValue<std::vector<int>>();
    auto whitelistR = bb.readValue<std::vector<int>>();

    if (blocklistR.isErr()) {
        return Err(ByteBuffer::strerror(blocklistR.unwrapErr()));
    }

    if (whitelistR.isErr()) {
        return Err(ByteBuffer::strerror(whitelistR.unwrapErr()));
    }

    auto blocklist = blocklistR.unwrap();
    auto whitelist = whitelistR.unwrap();

    for (int elem : blocklist) {
        _bl.insert(elem);
    }

    for (int elem : whitelist) {
        _wl.insert(elem);
    }

    return Ok();
}

bool BlockListManager::isExplicitlyBlocked(int playerId) {
    return _bl.contains(playerId);
}

bool BlockListManager::isExplicitlyAllowed(int playerId) {
    return _wl.contains(playerId);
}

void BlockListManager::blacklist(int playerId) {
    if (_wl.contains(playerId)) {
        _wl.erase(playerId);
    }
    _bl.insert(playerId);
    this->save();
}

void BlockListManager::whitelist(int playerId) {
    if (_bl.contains(playerId)) {
        _bl.erase(playerId);
    }
    _wl.insert(playerId);
    this->save();
}

bool BlockListManager::isHidden(int playerId) {
    return hiddenPlayers.contains(playerId);
}

void BlockListManager::setHidden(int playerId, bool state) {
    if (state) {
        hiddenPlayers.insert(playerId);
    } else {
        hiddenPlayers.erase(playerId);
    }
}
