#pragma once
#include <ui/BasePopup.hpp>
#include <Geode/utils/function.hpp>

namespace globed {

class EditServerPopup : public BasePopup {
public:
    using Callback = geode::Function<void(const std::string& name, const std::string& url)>;
    static EditServerPopup* create(bool, const std::string&, const std::string&);

    void setCallback(Callback&& fn);

protected:
    friend class BasePopup;
    static const cocos2d::CCSize POPUP_SIZE;

    geode::TextInput* m_nameInput;
    geode::TextInput* m_urlInput;
    Callback m_callback;

    bool init(bool, const std::string&, const std::string&);
};

}
