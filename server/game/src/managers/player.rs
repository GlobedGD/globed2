use rustc_hash::{FxHashMap, FxHashSet};

use crate::data::{
    types::{AssociatedPlayerData, PlayerData},
    AssociatedPlayerMetadata, PlayerMetadata,
};

#[derive(Default)]
pub struct PlayerEntry {
    pub data: AssociatedPlayerData,
    pub meta: AssociatedPlayerMetadata,
}

pub struct PlayerManager {
    players: FxHashMap<i32, PlayerEntry>,   // player id : associated data
    levels: FxHashMap<i32, FxHashSet<i32>>, // level id : [player id]
}

impl PlayerManager {
    pub fn new() -> Self {
        Self {
            players: FxHashMap::default(),
            levels: FxHashMap::default(),
        }
    }

    pub fn get_player_data(&self, account_id: i32) -> Option<&AssociatedPlayerData> {
        self.players.get(&account_id).map(|entry| &entry.data)
    }

    pub fn get_player_metadata(&self, account_id: i32) -> Option<&AssociatedPlayerMetadata> {
        self.players.get(&account_id).map(|entry| &entry.meta)
    }

    fn get_or_create_player(&mut self, account_id: i32) -> &mut PlayerEntry {
        self.players.entry(account_id).or_insert_with(|| {
            let mut entry = PlayerEntry::default();
            entry.data.account_id = account_id;
            entry.meta.account_id = account_id;
            entry
        })
    }

    /// set player's data, inserting a new entry if doesn't already exist
    pub fn set_player_data(&mut self, account_id: i32, data: &PlayerData) {
        self.get_or_create_player(account_id).data.data.clone_from(data);
    }

    /// set player's metadata, inserting a new entry if it doesn't already exist
    pub fn set_player_metadata(&mut self, account_id: i32, data: &PlayerMetadata) {
        self.get_or_create_player(account_id).meta.data.clone_from(data);
    }

    /// remove the player from the list of players
    pub fn remove_player(&mut self, account_id: i32) {
        self.players.remove(&account_id);
    }

    /// get a reference to a list of account IDs of players on a level given its ID
    pub fn get_level(&self, level_id: i32) -> Option<&FxHashSet<i32>> {
        self.levels.get(&level_id)
    }

    /// get the amount of players on a level given its ID
    pub fn get_player_count_on_level(&self, level_id: i32) -> Option<usize> {
        self.levels.get(&level_id).map(FxHashSet::len)
    }

    /// run a function `f` on each player on a level given its ID, with additional data (wink wink it's always gonna be `&mut FastByteBuffer`)
    pub fn for_each_player_on_level<F, A>(&self, level_id: i32, f: F, additional: &mut A) -> usize
    where
        F: Fn(&PlayerEntry, usize, &mut A) -> bool,
    {
        if let Some(ids) = self.levels.get(&level_id) {
            ids.iter()
                .filter_map(|&key| self.players.get(&key))
                .fold(0, |count, data| count + usize::from(f(data, count, additional)))
        } else {
            0
        }
    }

    /// add a player to a level given a level ID and an account ID
    pub fn add_to_level(&mut self, level_id: i32, account_id: i32) {
        let players = self.levels.entry(level_id).or_default();
        players.insert(account_id);
    }

    /// remove a player from a level given a level ID and an account ID
    pub fn remove_from_level(&mut self, level_id: i32, account_id: i32) {
        let should_remove_level = self.levels.get_mut(&level_id).is_some_and(|level| {
            level.remove(&account_id);
            level.is_empty()
        });

        if should_remove_level {
            self.levels.remove(&level_id);
        }
    }
}

impl Default for PlayerManager {
    fn default() -> Self {
        Self::new()
    }
}
