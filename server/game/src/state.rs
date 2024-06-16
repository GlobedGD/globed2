use crate::{
    managers::{RoleManager, RoomManager},
    util::WordFilter,
};
use std::sync::atomic::{AtomicU32, Ordering};

#[derive(Default)]
pub struct ServerState {
    pub player_count: AtomicU32,
    pub room_manager: RoomManager,
    pub role_manager: RoleManager,
    pub filter: WordFilter,
}

impl ServerState {
    pub fn new(filter_words: &[String]) -> Self {
        Self {
            filter: WordFilter::new(filter_words),
            ..Default::default()
        }
    }

    pub fn get_player_count(&self) -> u32 {
        self.player_count.load(Ordering::SeqCst)
    }

    pub fn inc_player_count(&self) {
        self.player_count.fetch_add(1, Ordering::SeqCst);
    }

    pub fn dec_player_count(&self) {
        self.player_count.fetch_sub(1, Ordering::SeqCst);
    }
}
