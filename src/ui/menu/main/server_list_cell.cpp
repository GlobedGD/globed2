#include "server_list_cell.hpp"
using namespace geode::prelude;

bool ServerListCell::init(const GameServerView& gsview) {
    if (!CCLayer::init()) return false;

    this->gsview = gsview;

    log::debug("list cell name: {}", gsview.name);

    return true;
}