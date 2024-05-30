use std::sync::atomic::{AtomicU8, Ordering};

#[repr(u8)]
#[derive(PartialEq, Eq, Clone, Copy, Debug)]
pub enum ClientThreadState {
    Unauthorized, // has an associated TCP stream but awaiting handshake / login
    Unclaimed,    // logged in, waiting to be claimed
    Disconnected, // does not have an associated TCP stream
    Established,  // connection has been fully established
    Terminating,  // thread is about to terminate
}

pub struct AtomicClientThreadState {
    val: AtomicU8,
}

impl AtomicClientThreadState {
    pub fn new(val: ClientThreadState) -> Self {
        Self {
            val: AtomicU8::new(val as u8),
        }
    }

    #[inline]
    pub fn store(&self, val: ClientThreadState) {
        self.store_with(val, Ordering::Relaxed);
    }

    #[inline]
    pub fn load(&self) -> ClientThreadState {
        self.load_with(Ordering::Relaxed)
    }

    #[inline]
    pub fn store_with(&self, val: ClientThreadState, ordering: Ordering) {
        self.val.store(val as u8, ordering);
    }

    #[inline]
    pub fn load_with(&self, ordering: Ordering) -> ClientThreadState {
        // safety: `store` is the only method that can write to `self.val`, and that only accepts valid enum values.
        unsafe { std::mem::transmute(self.val.load(ordering)) }
    }
}

impl Default for AtomicClientThreadState {
    fn default() -> Self {
        Self::new(ClientThreadState::Unauthorized)
    }
}
