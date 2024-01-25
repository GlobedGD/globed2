#pragma once
#include <defs.hpp>

#include <any>

class GlobedSettingCell : public cocos2d::CCLayer {
public:
    enum class Type {
        Bool, Int, Float, String, AudioDevice
    };

    static constexpr float CELL_WIDTH = 358.0f;
    static constexpr float CELL_HEIGHT = 45.0f;

    static GlobedSettingCell* create(void*, Type, const char*, const char*);

    void storeAndSave(std::any value);

private:
    void* settingStorage;
    Type settingType;
    const char *nameText, *descText;

    bool init(void*, Type, const char*, const char*);
};