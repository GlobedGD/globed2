#include "TwoPlayerModule.hpp"

using namespace geode::prelude;

namespace globed {

TwoPlayerModule::TwoPlayerModule() {}

void TwoPlayerModule::onModuleInit() {
    log::info("");
    this->setAutoEnableMode(AutoEnableMode::Level);
}

}
