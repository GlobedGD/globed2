#include "ScriptingUIModule.hpp"

using namespace geode::prelude;

namespace globed {

ScriptingUIModule::ScriptingUIModule() {}

void ScriptingUIModule::onModuleInit()
{
    log::info("Scripting UI module initialized");
    this->setAutoEnableMode(AutoEnableMode::Launch);
}

} // namespace globed
