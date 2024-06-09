use std::cell::SyncUnsafeCell;

/// Very simple wrapper around `SyncUnsafeCell` with nicer API.
#[repr(transparent)]
pub struct LockfreeMutCell<T> {
    cell: SyncUnsafeCell<T>,
}

impl<T> LockfreeMutCell<T> {
    pub const fn new(data: T) -> Self {
        Self {
            cell: SyncUnsafeCell::new(data),
        }
    }

    pub unsafe fn get(&self) -> &T {
        &*self.cell.get()
    }

    /// We trust you have received the usual lecture from the local System
    /// Administrator. It usually boils down to these three things:
    ///
    /// #1) Respect the privacy of others.
    ///
    /// #2) Think before you type.
    ///
    /// #3) With great power comes great responsibility.
    #[allow(clippy::mut_from_ref)]
    pub unsafe fn get_mut(&self) -> &mut T {
        &mut *self.cell.get()
    }

    pub unsafe fn swap(&self, new: T) -> T {
        std::mem::replace(self.get_mut(), new)
    }
}
