// Fun stuff.
// https://github.com/capnproto/capnproto/issues/2155
#include <stdexcept>

namespace kj {

class MonotonicClock;

const MonotonicClock &systemPreciseMonotonicClock()
{
    throw std::runtime_error("must not be called");
}

} // namespace kj