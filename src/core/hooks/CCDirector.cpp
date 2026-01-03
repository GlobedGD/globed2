#include <Geode/modify/CCDirector.hpp>
#include <Geode/Geode.hpp>
#include <core/net/NetworkManagerImpl.hpp>
#include <globed/config.hpp>

using namespace geode::prelude;

namespace globed {

struct GLOBED_MODIFY_ATTR HookedCCDirector : Modify<HookedCCDirector, CCDirector> {
    $override
    void purgeDirector() {
        CCDirector::purgeDirector();

        NetworkManagerImpl::get().shutdown();
    }
};

}