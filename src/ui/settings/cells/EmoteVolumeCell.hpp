#pragma once

#include "FloatSettingCell.hpp"

namespace globed {

class EmoteVolumeCell : public FloatSettingCell {
public:
    static EmoteVolumeCell* create(CStr key, CStr name, CStr desc, cocos2d::CCSize cellSize);

private:
    void setup() override;
    void playEmote();
};

}