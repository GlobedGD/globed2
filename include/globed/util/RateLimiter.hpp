#pragma once
#include <optional>
#include <asp/time/Instant.hpp>

namespace globed {

class RateLimiter {
public:
    static RateLimiter create(uint32_t perSec, uint32_t max) {
        // instead of 1 second, use 2^30 nanoseconds, so that if per_sec is a power of two, we can skip the division :)
        auto ns_to_refill_one = 1'073'741'824ULL / (uint64_t)perSec;

        return createPrecise(ns_to_refill_one, max);
    }

    static RateLimiter createPrecise(uint64_t ns_to_refill_one, uint32_t max) {
        ns_to_refill_one = ns_to_refill_one ?: 1;

        RateLimiter out;
        if ((ns_to_refill_one & (ns_to_refill_one - 1)) == 0) {
            out.m_pow2_shift = std::countr_zero(ns_to_refill_one);
        }
        out.m_ns_to_refill_one = ns_to_refill_one;
        out.m_max = max;
        out.m_tokens = max;
        out.m_last_refill_time = out.timestamp();

        return out;
    }

    static RateLimiter createUnlimited() {
        return createPrecise(1, UINT32_MAX);
    }

    /// Returns whether a token was available, false means rate limit was exceeded
    bool consume() {
        this->catchUp();

        if (m_tokens > 0) {
            m_tokens--;
            return true;
        }
        return false;
    }

private:
    uint32_t m_max;
    uint32_t m_tokens;
    uint64_t m_ns_to_refill_one;
    uint64_t m_last_refill_time;
    std::optional<uint32_t> m_pow2_shift;

    uint64_t timestamp() {
        return asp::Instant::now().rawNanos();
    }

    void catchUp() {
        auto now = timestamp();
        auto elapsed = now - m_last_refill_time;

        // convert to ticks
        if (m_pow2_shift) {
            elapsed >>= *m_pow2_shift;
        } else {
            elapsed /= m_ns_to_refill_one;
        }

        if (elapsed == 0) return;

        // for fairness, set to consumed * period instead of `now`, so nothing gets lost
        m_last_refill_time += elapsed * m_ns_to_refill_one;

        auto newt = (uint64_t)m_tokens + elapsed;
        m_tokens = std::min((uint32_t)newt, m_max);
    }
};

}