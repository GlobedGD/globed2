#pragma once

#include "FloatSettingCell.hpp"

namespace globed {

class EmoteVolumeCell : public FloatSettingCell {
public:
    static EmoteVolumeCell* create(ZStringView key, ZStringView name, ZStringView desc, CCSize cellSize);

private:
    void setup() override;
    void playEmote();
};

}