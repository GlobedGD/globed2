use globed_shared::{rand, rand::Rng, IntMap, SyncMutex, SyncMutexGuard};

use crate::data::ROOM_ID_LENGTH;

use super::LevelManager;

#[derive(Default)]
pub struct RoomManager {
    rooms: SyncMutex<IntMap<u32, LevelManager>>,
    global: SyncMutex<LevelManager>,
}

// i.e. if ROOM_ID_LENGTH is 6 we should have a range 100_000..1_000_000
const ROOM_ID_START: u32 = 10_u32.pow(ROOM_ID_LENGTH as u32 - 1);
const ROOM_ID_END: u32 = 10_u32.pow(ROOM_ID_LENGTH as u32);

impl RoomManager {
    pub fn new() -> Self {
        Self::default()
    }

    /// Try to find a room by given ID, if equal to 0 or not found, runs the provided closure with the global room
    pub fn with_any<F: FnOnce(&mut LevelManager) -> R, R>(&self, room_id: u32, f: F) -> R {
        if room_id == 0 {
            f(&mut self.get_global())
        } else if let Some(room) = self.get_rooms().get_mut(&room_id) {
            f(room)
        } else {
            f(&mut self.get_global())
        }
    }

    pub fn get_global(&self) -> SyncMutexGuard<'_, LevelManager> {
        self.global.lock()
    }

    pub fn get_rooms(&self) -> SyncMutexGuard<'_, IntMap<u32, LevelManager>> {
        self.rooms.lock()
    }

    /// Creates a new room, adds the given player, removes them from the global room, and returns the room ID
    pub fn create_room(&self, account_id: i32) -> u32 {
        let mut rooms = self.rooms.lock();

        // in case we accidentally generate an existing room id, keep looping until we find a suitable id
        let room_id = loop {
            let room_id = rand::thread_rng().gen_range(ROOM_ID_START..ROOM_ID_END);
            if !rooms.contains_key(&room_id) {
                break room_id;
            }
        };

        let mut pm = LevelManager::new();
        pm.create_player(account_id);

        rooms.insert(room_id, pm);
        self.get_global().remove_player(account_id);

        room_id
    }

    pub fn is_valid_room(&self, room_id: u32) -> bool {
        self.rooms.lock().contains_key(&room_id)
    }

    /// Deletes a room if there are no players in it
    pub fn maybe_remove_room(&self, room_id: u32) {
        let mut rooms = self.rooms.lock();

        let to_remove = rooms.get(&room_id).is_some_and(|room| room.get_total_player_count() == 0);

        if to_remove {
            rooms.remove(&room_id);
        }
    }
}
