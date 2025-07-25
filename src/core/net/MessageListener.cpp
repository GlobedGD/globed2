#include <globed/core/net/MessageListener.hpp>
#include "NetworkManagerImpl.hpp"

namespace globed {

void _destroyListener(const std::type_info& ty, void* ptr) {
    auto& nm = NetworkManagerImpl::get();
    nm.removeListener(ty, ptr);
}

}

