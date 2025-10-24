#pragma once

#include <ui/BaseLayer.hpp>
#include <cue/ListNode.hpp>
#include <globed/util/CStr.hpp>
#include "cells/BaseSettingCell.hpp"

namespace globed {

class SettingsLayer : public BaseLayer {
public:
    static SettingsLayer* create();
    static constexpr cocos2d::CCSize CELL_SIZE = {356.f, 32.f};

private:
    geode::Ref<cue::ListNode> m_list;

    bool init();
    void addSettings();

    template <typename T>
    T* addSetting(CStr key, CStr name, CStr desc) {
        auto cell = T::create(key, name, desc, CELL_SIZE);
        this->addSetting(cell);
        return cell;
    }

    void addHeader(CStr key, CStr text);
    void addSetting(BaseSettingCellBase* cell);

    void refreshAll();
};

}