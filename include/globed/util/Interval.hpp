#pragma once
#include <asp/time/Instant.hpp>

namespace globed {

/// Helper class for timing certain actions.
class Interval {
public:
    Interval(asp::time::Duration intvl) : m_next(asp::time::Instant::now()), m_interval(intvl) {}
    Interval() : Interval(asp::time::Duration::zero()) {}

    /// Returns whether work needs to be done, aka the interval has passed.
    bool tick()
    {
        return tick(asp::time::Instant::now());
    }

    /// Returns whether work needs to be done, aka the interval has passed.
    bool tick(const asp::time::Instant &now)
    {
        if (now < m_next || m_interval.isZero()) {
            return false;
        }

        do {
            m_next += m_interval;
        } while (now >= m_next && m_skip);

        return true;
    }

    /// Set whether the interval should skip missed ticks.
    /// If a long time passes between calls to tick() (longer than the set interval),
    /// and this is set to true (as by default), the ticks will be forgotten and the next tick()
    /// will be scheduled to run after the interval rather than immediately.
    void setSkipMissed(bool skip)
    {
        m_skip = skip;
    }

    void setInterval(asp::time::Duration intvl)
    {
        m_interval = intvl;
    }

    asp::time::Duration interval() const
    {
        return m_interval;
    }

private:
    asp::time::Instant m_next;
    asp::time::Duration m_interval;
    bool m_skip = true;
};

} // namespace globed