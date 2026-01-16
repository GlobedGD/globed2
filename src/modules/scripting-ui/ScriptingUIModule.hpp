#pragma once

#include <globed/core/ModuleCrtp.hpp>

namespace globed {

class ScriptingUIModule : public ModuleCrtpBase<ScriptingUIModule> {
public:
    ScriptingUIModule();

    void onModuleInit();

    virtual std::string_view name() const override
    {
        return "Scripting UI";
    }

    virtual std::string_view id() const override
    {
        return "globed.scripting-ui";
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
