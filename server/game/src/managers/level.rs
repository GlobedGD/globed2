use globed_shared::IntMap;

use crate::data::{
    types::PlayerData, AssociatedPlayerData, AssociatedPlayerMetadata, BorrowedAssociatedPlayerData,
    BorrowedAssociatedPlayerMetadata, LevelId, PlayerMetadata,
};

#[derive(Default)]
pub struct LevelManagerPlayer {
    pub account_id: i32,
    pub data: PlayerData,
    pub meta: PlayerMetadata,
}

impl LevelManagerPlayer {
    pub fn to_associated_data(&self) -> AssociatedPlayerData {
        AssociatedPlayerData {
            account_id: self.account_id,
            data: self.data.clone(),
        }
    }

    pub fn to_borrowed_associated_data(&self) -> BorrowedAssociatedPlayerData {
        BorrowedAssociatedPlayerData {
            account_id: self.account_id,
            data: &self.data,
        }
    }

    pub fn to_associated_meta(&self) -> AssociatedPlayerMetadata {
        AssociatedPlayerMetadata {
            account_id: self.account_id,
            data: self.meta.clone(),
        }
    }

    pub fn to_borrowed_associated_meta(&self) -> BorrowedAssociatedPlayerMetadata {
        BorrowedAssociatedPlayerMetadata {
            account_id: self.account_id,
            data: &self.meta,
        }
    }
}

// Manages an entire room (all levels and players inside of it).
#[derive(Default)]
pub struct LevelManager {
    pub players: IntMap<i32, LevelManagerPlayer>, // player id : associated data
    pub levels: IntMap<LevelId, Vec<i32>>,        // level id : [player id]
}

impl LevelManager {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn get_player_data(&self, account_id: i32) -> Option<&LevelManagerPlayer> {
        self.players.get(&account_id)
    }

    pub fn create_player(&mut self, account_id: i32) {
        self.players.insert(
            account_id,
            LevelManagerPlayer {
                account_id,
                ..Default::default()
            },
        );
    }

    fn get_or_create_player(&mut self, account_id: i32) -> &mut LevelManagerPlayer {
        self.players.entry(account_id).or_insert_with(|| LevelManagerPlayer {
            account_id,
            ..Default::default()
        })
    }

    /// set player's data, inserting a new entry if doesn't already exist
    pub fn set_player_data(&mut self, account_id: i32, data: &PlayerData) {
        self.get_or_create_player(account_id).data.clone_from(data);
    }

    /// set player's metadata, inserting a new entry if it doesn't already exist
    pub fn set_player_meta(&mut self, account_id: i32, meta: &PlayerMetadata) {
        self.get_or_create_player(account_id).meta.clone_from(meta);
    }

    /// remove the player from the list of players
    pub fn remove_player(&mut self, account_id: i32) {
        self.players.remove(&account_id);
    }

    /// get a reference to a list of account IDs of players on a level given its ID
    pub fn get_level(&self, level_id: LevelId) -> Option<&Vec<i32>> {
        self.levels.get(&level_id)
    }

    /// get amount of levels in the room
    pub fn get_level_count(&self) -> usize {
        self.levels.len()
    }

    /// get the amount of players on a level given its ID
    pub fn get_player_count_on_level(&self, level_id: LevelId) -> Option<usize> {
        self.levels.get(&level_id).map(Vec::len)
    }

    /// get the total amount of players
    pub fn get_total_player_count(&self) -> usize {
        self.players.len()
    }

    /// run a function `f` on each player on a level given its ID, with possibility to pass additional data
    pub fn for_each_player_on_level<F, A>(&self, level_id: LevelId, f: F, additional: &mut A) -> usize
    where
        F: Fn(&LevelManagerPlayer, usize, &mut A) -> bool,
    {
        if let Some(ids) = self.levels.get(&level_id) {
            ids.iter()
                .filter_map(|&key| self.players.get(&key))
                .fold(0, |count, data| count + usize::from(f(data, count, additional)))
        } else {
            0
        }
    }

    /// run a function `f` on each player in this `PlayerManager`, with possibility to pass additional data
    pub fn for_each_player<F, A>(&self, f: F, additional: &mut A) -> usize
    where
        F: Fn(&LevelManagerPlayer, usize, &mut A) -> bool,
    {
        self.players
            .values()
            .fold(0, |count, data| count + usize::from(f(data, count, additional)))
    }

    /// run a function `f` on each level in this `PlayerManager`, with possibility to pass additional data
    pub fn for_each_level<F, A>(&self, f: F, additional: &mut A) -> usize
    where
        F: Fn((LevelId, &Vec<i32>), usize, &mut A) -> bool,
    {
        self.levels.iter().fold(0, |count, (id, players)| {
            count + usize::from(f((*id, players), count, additional))
        })
    }

    /// add a player to a level given a level ID and an account ID
    pub fn add_to_level(&mut self, level_id: LevelId, account_id: i32) {
        let players = self.levels.entry(level_id).or_insert_with(|| Vec::with_capacity(8));
        if !players.contains(&account_id) {
            players.push(account_id);
        }
    }

    /// remove a player from a level given a level ID and an account ID
    pub fn remove_from_level(&mut self, level_id: LevelId, account_id: i32) {
        let should_remove_level = self.levels.get_mut(&level_id).is_some_and(|level| {
            if let Some(index) = level.iter().position(|&x| x == account_id) {
                level.remove(index);
            }

            level.is_empty()
        });

        if should_remove_level {
            self.levels.remove(&level_id);
        }
    }
}
