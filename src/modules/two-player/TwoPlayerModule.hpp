#pragma once

#include <globed/core/ModuleCrtp.hpp>

namespace globed {

class TwoPlayerModule : public ModuleCrtpBase<TwoPlayerModule> {
public:
    TwoPlayerModule();

    void onModuleInit();

    virtual std::string_view name() const override {
        return "Two Player Mode";
    }

    virtual std::string_view id() const override {
        return "globed.two-player-mode";
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
