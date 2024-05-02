use std::collections::HashMap;

use globed_shared::{warn, IntMap};
use serde::Deserialize;

#[derive(Deserialize)]
pub struct BakedResources {
    strings: HashMap<String, String>,
}

pub struct ResourceManager {
    strings: IntMap<u32, String>,
}

impl ResourceManager {
    pub fn new() -> Self {
        let content = include_str!("../../../../baked-resources.json");

        Self {
            strings: Self::init_from_data(content),
        }
    }

    pub fn get_string(&self, key: &str) -> &str {
        self.get_string_by_hash(Self::hash(key))
    }

    pub fn get_string_by_hash(&self, hash: u32) -> &str {
        self.strings.get(&hash).map_or("<invalid string>", |x| x.as_str())
    }

    fn init_from_data(data: &str) -> IntMap<u32, String> {
        let data = match serde_json::from_str::<BakedResources>(data) {
            Ok(x) => x,
            Err(e) => {
                warn!("failed to parse baked-resources.json: {}", e);
                panic!("could not parse baked resources");
            }
        };

        let mut map = IntMap::default();

        for (key, val) in data.strings {
            map.insert(Self::hash(&key), val);
        }

        map
    }

    fn hash(key: &str) -> u32 {
        esp::hash::adler32(key.as_bytes())
    }
}

impl Default for ResourceManager {
    fn default() -> Self {
        Self::new()
    }
}
