#include "UIModule.hpp"

using namespace geode::prelude;

namespace globed {

UIModule::UIModule() {}

void UIModule::onModuleInit()
{
    this->setAutoEnableMode(AutoEnableMode::Server);
}

} // namespace globed
