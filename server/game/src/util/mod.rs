pub mod channel;
pub mod lockfreemutcell;
pub mod rate_limiter;

pub use channel::{SenderDropped, TokioChannel};
pub use lockfreemutcell::LockfreeMutCell;
pub use rate_limiter::SimpleRateLimiter;

/// Creates a new array `[u8; N]` on the stack with uninitiailized memory
#[macro_export]
macro_rules! new_uninit {
    ($size:expr) => {{
        let mut arr: [std::mem::MaybeUninit<u8>; $size] = std::mem::MaybeUninit::uninit().assume_init();
        make_uninit!(&mut arr)
    }};
}

/// Converts a `&[MaybeUninit<u8>]` into `&mut [u8]`
#[macro_export]
macro_rules! make_uninit {
    ($slc:expr) => {{
        let arr_ptr = $slc.as_mut_ptr().cast::<u8>();
        std::slice::from_raw_parts_mut(arr_ptr, std::mem::size_of_val($slc))
    }};
}
