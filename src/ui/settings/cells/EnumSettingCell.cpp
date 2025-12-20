#include "EnumSettingCell.hpp"

using namespace geode::prelude;

namespace globed {

void EnumSettingCell::setup() {
    m_btnText = "";
    m_callback = [this] {
        this->pickNext();
    };

    ButtonSettingCell::setup();

    this->reload();
}

void EnumSettingCell::reload() {
    m_current = this->get<int>();

    bool found = false;
    for (size_t i = 0; i < m_options.size(); ++i) {
        auto& option = m_options[i];
        if (option.second == m_current) {
            m_btnText = option.first;
            m_currentIdx = i;
            found = true;
            break;
        }
    }

    if (!found) {
        auto& opt = m_options.front();
        m_current = opt.second;
        m_currentIdx = 0;
        m_btnText = opt.first;
        this->set(m_current);
    }

    this->createButton(m_btnText);
}

void EnumSettingCell::pickNext() {
    auto nextIdx = (m_currentIdx + 1) % m_options.size();
    auto nextVal = m_options[nextIdx].second;
    this->set(nextVal);
    this->reload();
}

EnumSettingCell* EnumSettingCell::create(CStr key, CStr name, CStr desc, std::vector<std::pair<CStr, int>> options, CCSize cellSize) {
    GLOBED_ASSERT(!options.empty());

    auto ret = new EnumSettingCell;
    ret->m_options = std::move(options);
    if (ret->init(key, name, desc, cellSize)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

}