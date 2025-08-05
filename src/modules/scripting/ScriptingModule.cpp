#include "ScriptingModule.hpp"

using namespace geode::prelude;

namespace globed {

ScriptingModule::ScriptingModule() {}

void ScriptingModule::onModuleInit() {
    log::info("Scripting module initialized");
    this->setAutoEnableMode(AutoEnableMode::Level);
}

}
