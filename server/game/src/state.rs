use crate::managers::{ResourceManager, RoomManager};
use std::sync::atomic::AtomicU32;

#[derive(Default)]
pub struct ServerState {
    pub player_count: AtomicU32,
    pub room_manager: RoomManager,
    pub resource_manager: ResourceManager,
}

impl ServerState {
    pub fn new() -> Self {
        Self::default()
    }
}
