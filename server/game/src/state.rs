use crate::managers::RoomManager;
use std::sync::atomic::AtomicU32;

#[derive(Default)]
pub struct ServerState {
    pub player_count: AtomicU32,
    pub room_manager: RoomManager,
}

impl ServerState {
    pub fn new() -> Self {
        Self::default()
    }
}
