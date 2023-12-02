use std::{
    cell::SyncUnsafeCell,
    time::{Duration, Instant},
};

/// Naive rate limiter implementation, cannot sleep and is not thread safe on its own.
pub struct SimpleRateLimiter {
    limit: usize,
    count: usize,
    period: Duration,
    last_refill: Instant,
}

impl SimpleRateLimiter {
    pub fn new(limit: usize, period: Duration) -> Self {
        Self {
            limit,
            count: limit,
            period,
            last_refill: Instant::now(),
        }
    }

    /// Returns `true` if we are not ratelimited, `false` if we are.
    pub fn try_tick(&mut self) -> bool {
        if self.count > 0 {
            self.count -= 1;
            true
        } else {
            let now = Instant::now();
            if (now - self.last_refill) > self.period {
                self.count = self.limit;
                self.last_refill = now;
                true
            } else {
                false
            }
        }
    }
}

/// A ratelimiter wrapped in a `SyncUnsafeCell`, does not need to be mutable to use.
/// If two different threads attempt to call `try_tick()` on the same instance at the same time,
/// the behavior is undefined.
#[repr(transparent)]
pub struct UnsafeRateLimiter {
    limiter: SyncUnsafeCell<SimpleRateLimiter>,
}

impl UnsafeRateLimiter {
    pub fn new(limiter: SimpleRateLimiter) -> Self {
        Self {
            limiter: SyncUnsafeCell::new(limiter),
        }
    }

    /// Returns `true` if we are not ratelimited, `false` if we are.
    /// Calling this from multiple threads at the same time is **undefined behavior**.
    pub unsafe fn try_tick(&self) -> bool {
        // UnsafeCell can never be nullptr, so the unwrap is safe.
        self.limiter.get().as_mut().unwrap_unchecked().try_tick()
    }
}
