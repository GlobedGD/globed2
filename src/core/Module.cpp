#include <Geode/utils/terminate.hpp>
#include <globed/core/Module.hpp>

using namespace geode::prelude;

namespace globed {

Result<> Module::enable()
{
    this->assertCore();

    if (this->m_enabled) {
        return Ok();
    }

    GEODE_UNWRAP(this->onEnabled());
    GEODE_UNWRAP(this->enableHooks());

    m_enabled = true;
    return Ok();
}

Result<> Module::disable()
{
    this->assertCore();

    if (!this->m_enabled) {
        return Ok();
    }

    m_enabled = false;

    GEODE_UNWRAP(this->disableHooks());
    GEODE_UNWRAP(this->onDisabled());

    return Ok();
}

bool Module::isEnabled() const
{
    return m_enabled;
}

Result<> Module::enableHooks()
{
    for (auto &hook : m_hooks) {
        // note that we dont have to check if the hook is already enabled, geode just returns Ok, unlike patches
        GEODE_UNWRAP(hook->enable());
    }

    for (auto &patch : m_patches) {
        if (!patch->isEnabled())
            GEODE_UNWRAP(patch->enable());
    }

    return Ok();
}

Result<> Module::disableHooks()
{
    for (auto &hook : m_hooks) {
        GEODE_UNWRAP(hook->disable());
    }

    for (auto &patch : m_patches) {
        if (patch->isEnabled())
            GEODE_UNWRAP(patch->disable());
    }

    return Ok();
}

void Module::assertCore() const
{
    if (!m_core) {
        geode::utils::terminate(
            fmt::format("Globed module {} ({}) by {} called a module function before registering the mod with the core",
                        this->name(), this->id(), this->author()),
            Mod::get());
    }
}

} // namespace globed