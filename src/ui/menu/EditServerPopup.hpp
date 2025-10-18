#pragma once
#include <ui/BasePopup.hpp>

#include <std23/move_only_function.h>

namespace globed {

class EditServerPopup : public BasePopup<EditServerPopup, bool, const std::string&, const std::string&> {
public:
    using Callback = std23::move_only_function<void(const std::string& name, const std::string& url)>;

    void setCallback(Callback&& fn);

protected:
    friend class BasePopup;
    static const cocos2d::CCSize POPUP_SIZE;

    geode::TextInput* m_nameInput;
    geode::TextInput* m_urlInput;
    Callback m_callback;

    bool setup(bool, const std::string&, const std::string&) override;
};

}
