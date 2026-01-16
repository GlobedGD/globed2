#pragma once

#include <globed/core/ModuleCrtp.hpp>

namespace globed {

class UIModule : public ModuleCrtpBase<UIModule> {
public:
    UIModule();

    void onModuleInit();

    virtual std::string_view name() const override
    {
        return "UI Module";
    }

    virtual std::string_view id() const override
    {
        return "globed.ui";
    }

    virtual std::string_view author() const override
    {
        return "Globed";
    }

    virtual std::string_view description() const override
    {
        return "";
    }

private:
};

} // namespace globed
