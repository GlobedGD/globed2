#include "ScriptingUIModule.hpp"

using namespace geode::prelude;

namespace globed {

ScriptingUIModule::ScriptingUIModule() {
    log::info("Scripting UI module initialized");
    this->setAutoEnableMode(AutoEnableMode::Launch);
}

}
