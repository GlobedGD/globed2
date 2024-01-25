#pragma once
#include <defs.hpp>

class GlobedSettingHeaderCell : public cocos2d::CCLayer {
public:
    static GlobedSettingHeaderCell* create(const char* name);

private:
    const char* name;

    bool init(const char* name);
};