#include "singleton.hpp"

class singleton_use_after_dtor : public std::runtime_error {
public:
    singleton_use_after_dtor() : std::runtime_error("attempting to use a singleton after static destruction") {}
};

namespace globed {
    void destructedSingleton() {
        throw singleton_use_after_dtor();
    }
}
