#include "setting_cell.hpp"

#include <managers/settings.hpp>

using namespace geode::prelude;

bool GlobedSettingCell::init(void* settingStorage, Type settingType, const char* nameText, const char* descText) {
    if (!CCLayer::init()) return false;
    this->settingStorage = settingStorage;
    this->settingType = settingType;
    this->nameText = nameText;
    this->descText = descText;

    return true;
}

void GlobedSettingCell::storeAndSave(std::any value) {
    // banger
    switch (settingType) {
    case Type::Bool:
        *(bool*)(settingStorage) = std::any_cast<bool>(value); break;
    case Type::Float:
        *(float*)(settingStorage) = std::any_cast<float>(value); break;
    case Type::String:
        *(std::string*)(settingStorage) = std::any_cast<std::string>(value); break;
    case Type::AudioDevice: [[fallthrough]];
    case Type::Int:
        *(int*)(settingStorage) = std::any_cast<int>(value); break;
    }

    GlobedSettings::get().save();
}

GlobedSettingCell* GlobedSettingCell::create(void* settingStorage, Type settingType, const char* name, const char* desc) {
    auto ret = new GlobedSettingCell;
    if (ret->init(settingStorage, settingType, name, desc)) {
        return ret;
    }

    delete ret;
    return nullptr;
}