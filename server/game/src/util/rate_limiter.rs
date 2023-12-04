use std::time::{Duration, Instant};

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

    /// Returns the amount of time since last refill
    pub fn since_last_refill(&self) -> Duration {
        self.last_refill.elapsed()
    }
}
