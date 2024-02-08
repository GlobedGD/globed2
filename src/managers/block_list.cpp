#include "block_list.hpp"

#include <data/bytebuffer.hpp>
#include <util/crypto.hpp>

BlockListMangaer::BlockListMangaer() {
    try {
        this->load();
    } catch (const std::exception& e) {
        _bl.clear();
        _wl.clear();
    }
}

void BlockListMangaer::save() {
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

    auto data = util::crypto::base64Encode(bb.getDataRef());

    Mod::get()->setSavedValue(SETTING_KEY, data);
}

void BlockListMangaer::load() {
    _bl.clear();
    _wl.clear();

    auto val = Mod::get()->getSavedValue<std::string>(SETTING_KEY);
    if (val.empty()) return;

    util::data::bytevector data;

    try {
        data = util::crypto::base64Decode(val);
    } catch (const std::exception& e) {
        log::warn("Failed to load blocklist: {}", e.what());
        return;
    }

    if (data.empty()) return;

    ByteBuffer bb(std::move(data));

    size_t blsize = bb.readU32();
    for (size_t i = 0; i < blsize; i++) {
        _bl.insert(bb.readI32());
    }

    size_t wlsize = bb.readU32();
    for (size_t i = 0; i < wlsize; i++) {
        _wl.insert(bb.readI32());
    }
}

bool BlockListMangaer::isExplicitlyBlocked(int playerId) {
    return _bl.contains(playerId);
}

bool BlockListMangaer::isExplicitlyAllowed(int playerId) {
    return _wl.contains(playerId);
}

void BlockListMangaer::blacklist(int playerId) {
    if (_wl.contains(playerId)) {
        _wl.erase(playerId);
    }
    _bl.insert(playerId);
    this->save();
}

void BlockListMangaer::whitelist(int playerId) {
    if (_bl.contains(playerId)) {
        _bl.erase(playerId);
    }
    _wl.insert(playerId);
    this->save();
}