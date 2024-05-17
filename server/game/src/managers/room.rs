use globed_shared::{
    rand::{self, Rng},
    IntMap, SyncMutex, SyncMutexGuard,
};

use crate::data::{LevelId, RoomInfo, RoomListingInfo, RoomSettings, ROOM_ID_LENGTH};

use super::LevelManager;

#[derive(Default)]
pub struct Room {
    pub owner: i32,
    pub token: u32,
    pub manager: LevelManager,
    pub settings: RoomSettings,
}

#[derive(Default)]
pub struct RoomManager {
    rooms: SyncMutex<IntMap<u32, Room>>,
    global: SyncMutex<Room>,
}

// i.e. if ROOM_ID_LENGTH is 6 we should have a range 100_000..1_000_000
const ROOM_ID_START: u32 = 10_u32.pow(ROOM_ID_LENGTH as u32 - 1);
const ROOM_ID_END: u32 = 10_u32.pow(ROOM_ID_LENGTH as u32);

impl Room {
    pub fn new(owner: i32, manager: LevelManager) -> Self {
        Self {
            owner,
            token: rand::random(),
            manager,
            settings: RoomSettings::default(),
        }
    }

    // Removes a player, if the player was the owner, rotates the owner and returns `true`.
    pub fn remove_player(&mut self, player: i32) -> bool {
        let was_owner = self.owner == player;

        if was_owner {
            // rotate the owner
            let mut rotate_to: i32 = 0;
            self.manager.for_each_player(
                |rp, _, rotate_to| {
                    if *rotate_to == 0 && rp.account_id != player {
                        *rotate_to = rp.account_id;
                    }
                    true
                },
                &mut rotate_to,
            );

            self.owner = rotate_to;
        }

        self.manager.remove_player(player);

        was_owner
    }

    #[inline]
    pub fn set_settings(&mut self, settings: &RoomSettings) {
        self.settings.clone_from(settings);
    }

    #[inline]
    pub fn get_room_info(&self, id: u32) -> RoomInfo {
        RoomInfo {
            id,
            token: self.token,
            owner: self.owner,
            settings: self.settings,
        }
    }

    #[inline]
    pub fn get_room_listing_info(&self, id: u32) -> RoomListingInfo {
        RoomListingInfo {
            id,
            owner: self.owner,
            settings: self.settings,
        }
    }

    pub fn is_invite_only(&self) -> bool {
        self.settings.flags.invite_only
    }

    pub fn is_public_invites(&self) -> bool {
        self.settings.flags.public_invites
    }

    pub fn is_two_player_mode(&self) -> bool {
        self.settings.flags.two_player
    }
}

impl RoomManager {
    pub fn new() -> Self {
        Self::default()
    }

    /// Try to find a room by given ID, if equal to 0 or not found, runs the provided closure with the global room
    pub fn with_any<F: FnOnce(&mut Room) -> R, R>(&self, room_id: u32, f: F) -> R {
        if room_id == 0 {
            f(&mut self.get_global())
        } else if let Some(room) = self.get_rooms().get_mut(&room_id) {
            f(room)
        } else {
            f(&mut self.get_global())
        }
    }

    /// Try to find a room by given ID and run the provided function on it. If not found, calls `default`.
    pub fn try_with_any<F: FnOnce(&mut Room) -> R, D: FnOnce() -> R, R>(&self, room_id: u32, f: F, default: D) -> R {
        if room_id == 0 {
            f(&mut self.get_global())
        } else if let Some(room) = self.get_rooms().get_mut(&room_id) {
            f(room)
        } else {
            default()
        }
    }

    pub fn get_global(&self) -> SyncMutexGuard<'_, Room> {
        self.global.lock()
    }

    pub fn get_rooms(&self) -> SyncMutexGuard<'_, IntMap<u32, Room>> {
        self.rooms.lock()
    }

    /// Creates a new room, adds the given player, removes them from the global room, and returns the room ID
    pub fn create_room(&self, account_id: i32) -> RoomInfo {
        let rooms = self.rooms.lock();

        // in case we accidentally generate an existing room id, keep looping until we find a suitable id
        let room_id = loop {
            let room_id = rand::thread_rng().gen_range(ROOM_ID_START..ROOM_ID_END);
            if !rooms.contains_key(&room_id) {
                break room_id;
            }
        };

        drop(rooms);

        let room = self._create_room(room_id, account_id);

        self.get_global().remove_player(account_id);

        room
    }

    pub fn is_valid_room(&self, room_id: u32) -> bool {
        self.rooms.lock().contains_key(&room_id)
    }

    /// Deletes a room if there are no players in it
    pub fn maybe_remove_room(&self, room_id: u32) {
        let mut rooms = self.rooms.lock();

        let to_remove = rooms.get(&room_id).is_some_and(|room| room.manager.get_total_player_count() == 0);

        if to_remove {
            rooms.remove(&room_id);
        }
    }

    // Removes the player from the given room, returns `true` if the player was the owner of the room,
    // and either a new owner has now been chosen, or the room has been deleted.
    pub fn remove_with_any(&self, room_id: u32, account_id: i32, level_id: LevelId) -> bool {
        let was_owner = self.with_any(room_id, |pm| {
            let was_owner = pm.remove_player(account_id);

            if level_id != 0 {
                pm.manager.remove_from_level(level_id, account_id);
            }

            was_owner
        });

        // delete the room if there are no more players there
        if room_id != 0 {
            self.maybe_remove_room(room_id);
        }

        was_owner
    }

    pub fn get_room_info(&self, room_id: u32) -> RoomInfo {
        self.try_with_any(
            room_id,
            |room| RoomInfo {
                id: room_id,
                owner: room.owner,
                token: room.token,
                settings: room.settings,
            },
            || RoomInfo {
                id: 0,
                owner: 0,
                token: 0,
                settings: RoomSettings::default(),
            },
        )
    }

    fn _create_room(&self, room_id: u32, owner: i32) -> RoomInfo {
        let mut rooms = self.rooms.lock();

        let mut pm = LevelManager::new();
        if owner != 0 {
            pm.create_player(owner);
        }

        let room = Room::new(owner, pm);

        let room_info = RoomInfo {
            id: room_id,
            owner,
            token: room.token,
            settings: room.settings,
        };

        rooms.insert(room_id, room);

        room_info
    }
}
