#include "setting_header_cell.hpp"

bool GlobedSettingHeaderCell::init(const char* name) {
    if (!CCLayer::init()) return false;
    this->name = name;

    return true;
}

GlobedSettingHeaderCell* GlobedSettingHeaderCell::create(const char* name) {
    auto ret = new GlobedSettingHeaderCell;
    if (ret->init(name)) {
        return ret;
    }

    delete ret;
    return nullptr;
}