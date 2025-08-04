#pragma once

#include <globed/core/ModuleCrtp.hpp>

namespace globed {

class ScriptingModule : public ModuleCrtpBase<ScriptingModule> {
public:
    ScriptingModule();

    void onModuleInit();

    virtual std::string_view name() const override {
        return "Scripting";
    }

    virtual std::string_view id() const override {
        return "globed.scripting";
    }

    virtual std::string_view author() const override {
        return "Globed";
    }

    virtual std::string_view description() const override {
        return "";
    }
private:
};

}
