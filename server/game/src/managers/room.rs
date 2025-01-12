use std::sync::{
    Arc, OnceLock,
    atomic::{AtomicI32, Ordering},
};

use esp::InlineString;
use globed_shared::{
    IntMap, SyncMutex, SyncMutexGuard, SyncRwLock,
    rand::{self, Rng},
};

use crate::{
    data::{LevelId, PlayerPreviewAccountData, ROOM_ID_LENGTH, RoomInfo, RoomListingInfo, RoomSettings},
    server::GameServer,
};

use super::LevelManager;

#[derive(Default)]
struct RoomMutableData {
    pub owner: Option<PlayerPreviewAccountData>,
    pub settings: RoomSettings,
}

#[derive(Default)]
pub struct Room {
    pub orig_owner: i32,
    pub orig_owner_data: Option<PlayerPreviewAccountData>,
    pub owner: AtomicI32,
    pub name: InlineString<32>,
    pub password: InlineString<16>,
    pub manager: SyncRwLock<LevelManager>,
    pub id: u32,
    data: SyncMutex<RoomMutableData>,
}

#[derive(Default)]
pub struct RoomManager {
    rooms: SyncMutex<IntMap<u32, Arc<Room>>>,
    global: Arc<Room>,
    game_server: OnceLock<&'static GameServer>,
}

// i.e. if ROOM_ID_LENGTH is 6 we should have a range 100_000..1_000_000
const ROOM_ID_START: u32 = 10_u32.pow(ROOM_ID_LENGTH as u32 - 1);
const ROOM_ID_END: u32 = 10_u32.pow(ROOM_ID_LENGTH as u32);

impl Room {
    pub fn new(
        owner: i32,
        owner_data: Option<PlayerPreviewAccountData>,
        name: InlineString<32>,
        password: InlineString<16>,
        settings: RoomSettings,
        manager: LevelManager,
        id: u32,
    ) -> Self {
        Self {
            orig_owner: owner,
            orig_owner_data: owner_data.clone(),
            owner: AtomicI32::new(owner),
            name,
            password,
            manager: SyncRwLock::new(manager),
            id,
            data: SyncMutex::new(RoomMutableData { owner: owner_data, settings }),
        }
    }

    // Removes a player, if the player was the owner, rotates the owner and returns `true`.
    pub fn remove_player(&self, player: i32) -> bool {
        let was_owner = self.get_owner() == player;

        let mut manager = self.manager.write();

        if was_owner {
            // rotate the owner
            let mut rotate_to: i32 = 0;
            manager.for_each_player(|rp| {
                if rotate_to == 0 && rp.account_id != player {
                    rotate_to = rp.account_id;
                }
            });

            self.owner.store(rotate_to, Ordering::Relaxed);
        }

        manager.remove_player(player);

        was_owner
    }

    /// Removes a player, does not rotate the room host. Be careful, only use this for the global room or if you're certain the player isn't the room host.
    pub fn remove_player_no_rotate(&self, player: i32) {
        self.manager.write().remove_player(player);
    }

    #[inline]
    pub fn has_player(&self, player: i32) -> bool {
        self.manager.read().has_player(player)
    }

    #[inline]
    pub fn set_settings(&self, settings: RoomSettings) {
        self.data.lock().settings = settings;
    }

    #[inline]
    pub fn get_room_info(&self) -> RoomInfo {
        let data = self.data.lock();

        RoomInfo {
            id: self.id,
            name: self.name.clone(),
            password: self.password.clone(),
            owner: data.owner.clone().unwrap_or_default(),
            settings: data.settings,
        }
    }

    #[inline]
    pub fn get_room_listing_info(&self, id: u32) -> RoomListingInfo {
        let data = self.data.lock();
        let player_count = self.get_player_count() as u16;

        RoomListingInfo {
            id,
            player_count,
            name: self.name.clone(),
            has_password: !self.password.is_empty(),
            owner: data.owner.clone().unwrap_or_default(),
            settings: data.settings,
        }
    }

    pub fn is_hidden(&self) -> bool {
        self.data.lock().settings.flags.is_hidden
    }

    pub fn is_public_invites(&self) -> bool {
        self.data.lock().settings.flags.public_invites
    }

    pub fn is_two_player_mode(&self) -> bool {
        self.data.lock().settings.flags.two_player
    }

    pub fn is_protected(&self) -> bool {
        !self.password.is_empty()
    }

    pub fn verify_password(&self, pwd: &InlineString<16>) -> bool {
        self.password.is_empty() || self.password == *pwd
    }

    pub fn is_full(&self) -> bool {
        let player_count = self.get_player_count();
        let data = self.data.lock();

        if data.settings.player_limit == 0 {
            false
        } else {
            player_count >= (data.settings.player_limit as usize)
        }
    }

    pub fn get_player_count(&self) -> usize {
        self.manager.read().get_total_player_count()
    }

    pub fn get_player_count_on_level(&self, level_id: LevelId) -> Option<usize> {
        self.manager.read().get_player_count_on_level(level_id)
    }

    pub fn get_level_count(&self) -> usize {
        self.manager.read().get_level_count()
    }

    pub fn get_owner(&self) -> i32 {
        self.owner.load(Ordering::Relaxed)
    }
}

impl RoomManager {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn set_game_server(&self, game_server: &'static GameServer) {
        self.game_server.set(game_server).ok().expect("set_game_server failed");
    }

    #[inline]
    fn get_game_server(&self) -> &'static GameServer {
        self.game_server
            .get()
            .expect("get_game_server called without a previous call to set_game_server")
    }

    /// Try to find a room by given ID, if equal to 0 or not found, runs the provided closure with the global room
    pub fn with_any<F: FnOnce(&Room) -> R, R>(&self, room_id: u32, f: F) -> R {
        f(&self.get_room_or_global(room_id))
    }

    /// Try to find a room by given ID and run the provided function on it. If not found, calls `default`.
    pub fn try_with_any<F: FnOnce(&Room) -> R, D: FnOnce() -> R, R>(&self, room_id: u32, f: F, default: D) -> R {
        if room_id == 0 {
            f(self.get_global())
        } else {
            let room = self.get_room(room_id);
            if let Some(room) = room { f(&room) } else { default() }
        }
    }

    pub fn get_global(&self) -> &Room {
        &self.global
    }

    pub fn get_global_owned(&self) -> Arc<Room> {
        self.global.clone()
    }

    pub fn get_rooms(&self) -> SyncMutexGuard<'_, IntMap<u32, Arc<Room>>> {
        self.rooms.lock()
    }

    pub fn get_room(&self, room_id: u32) -> Option<Arc<Room>> {
        self.rooms.lock().get(&room_id).cloned()
    }

    pub fn get_room_or_global(&self, room_id: u32) -> Arc<Room> {
        if room_id == 0 {
            self.get_global_owned()
        } else {
            self.get_room(room_id).unwrap_or_else(|| self.get_global_owned())
        }
    }

    /// Creates a new room, adds the given player, removes them from the global room, and returns the room
    pub fn create_room(&self, account_id: i32, name: InlineString<32>, password: InlineString<16>, settings: RoomSettings) -> Arc<Room> {
        let rooms = self.rooms.lock();

        // in case we accidentally generate an existing room id, keep looping until we find a suitable id
        let room_id = loop {
            let room_id = rand::rng().random_range(ROOM_ID_START..ROOM_ID_END);
            if !rooms.contains_key(&room_id) {
                break room_id;
            }
        };

        drop(rooms);

        let room = self._create_room(room_id, account_id, name, password, settings);

        self.get_global().remove_player(account_id); // dont worry about setting owner data here

        room
    }

    pub fn is_valid_room(&self, room_id: u32) -> bool {
        self.rooms.lock().contains_key(&room_id)
    }

    /// Deletes a room if there are no players in it
    pub fn maybe_remove_room(&self, room_id: u32) {
        let mut rooms = self.rooms.lock();

        let to_remove = rooms.get(&room_id).is_some_and(|room| room.get_player_count() == 0);

        if to_remove {
            rooms.remove(&room_id);
        }
    }

    // Removes the player from the given room, returns `true` if the player was the owner of the room,
    // and either a new owner has now been chosen, or the room has been deleted.
    pub fn remove_player(&self, room: &Room, account_id: i32, level_id: LevelId) -> bool {
        let was_owner = room.remove_player(account_id);

        if was_owner {
            // refresh owner data to newest
            room.data.lock().owner = self.get_game_server().get_player_preview_data(room.owner.load(Ordering::Relaxed));
        }

        if level_id != 0 {
            room.manager.write().remove_from_level(level_id, account_id);
        }

        // delete the room if there are no more players there
        if room.id != 0 {
            self.maybe_remove_room(room.id);
        }

        was_owner
    }

    pub fn get_room_info(&self, room_id: u32) -> Option<RoomInfo> {
        self.try_with_any(room_id, |room| Some(room.get_room_info()), || None)
    }

    fn _create_room(&self, room_id: u32, owner: i32, name: InlineString<32>, password: InlineString<16>, settings: RoomSettings) -> Arc<Room> {
        let mut rooms = self.rooms.lock();

        let mut pm = LevelManager::new();
        if owner != 0 {
            let owner_thread = self.get_game_server().get_user_by_id(owner);
            let is_invisible = owner_thread.map(|thr| thr.privacy_settings.lock().get_hide_in_game()).unwrap_or(false);

            pm.create_player(owner, is_invisible);
        }

        let owner_data = self.get_game_server().get_player_preview_data(owner);

        let room = Room::new(owner, owner_data, name, password, settings, pm, room_id);
        let room = Arc::new(room);

        rooms.insert(room_id, room.clone());

        room
    }
}
