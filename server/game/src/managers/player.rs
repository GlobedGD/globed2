use rustc_hash::{FxHashMap, FxHashSet};

use crate::data::types::{AssociatedPlayerData, PlayerData};

pub struct PlayerManager {
    players: FxHashMap<i32, AssociatedPlayerData>, // player id : associated data
    levels: FxHashMap<i32, FxHashSet<i32>>,        // level id : [player id]
}

impl PlayerManager {
    pub fn new() -> Self {
        Self {
            players: FxHashMap::default(),
            levels: FxHashMap::default(),
        }
    }

    pub fn get_player(&self, account_id: i32) -> Option<&AssociatedPlayerData> {
        self.players.get(&account_id)
    }

    pub fn get_players(&self, account_ids: &[i32]) -> Vec<&AssociatedPlayerData> {
        account_ids.iter().filter_map(|&key| self.players.get(&key)).collect()
    }

    pub fn set_player_data(&mut self, account_id: i32, data: &PlayerData) {
        let entry = self.players.entry(account_id).or_default();
        entry.account_id = account_id;
        entry.data.clone_from(data);
    }

    pub fn remove_player(&mut self, account_id: i32) {
        self.players.remove(&account_id);
    }

    pub fn get_level(&self, level_id: i32) -> Option<&FxHashSet<i32>> {
        self.levels.get(&level_id)
    }

    pub fn get_players_on_level(&self, level_id: i32) -> Option<Vec<&AssociatedPlayerData>> {
        let ids = self.levels.get(&level_id)?;
        Some(ids.iter().filter_map(|&key| self.players.get(&key)).collect())
    }

    pub fn add_to_level(&mut self, level_id: i32, account_id: i32) {
        let players = self.levels.entry(level_id).or_default();
        players.insert(account_id);
    }

    pub fn remove_from_level(&mut self, level_id: i32, account_id: i32) {
        self.levels.get_mut(&level_id).map(|level| level.remove(&account_id));
    }
}

impl Default for PlayerManager {
    fn default() -> Self {
        Self::new()
    }
}
