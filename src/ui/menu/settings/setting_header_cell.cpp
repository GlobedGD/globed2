#include "setting_header_cell.hpp"

bool GlobedSettingHeaderCell::init(const char* name) {
    if (!CCLayer::init()) return false;

    Build<cocos2d::CCLabelBMFont>::create(name, "goldFont.fnt")
        .scale(0.8f)
        .pos(CELL_WIDTH / 2, CELL_HEIGHT / 2)
        .parent(this)
        .store(label);

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