use crate::managers::PlayerManager;
use parking_lot::Mutex as SyncMutex;
use std::sync::atomic::AtomicU32;
use tokio::sync::{RwLock, RwLockReadGuard, RwLockWriteGuard};

pub struct ServerStateInner {
    pub http_client: reqwest::Client,
    pub central_url: String,
    pub central_pw: String,
}

pub struct ServerState {
    pub player_count: AtomicU32,
    // make player_manager SyncRwLock if there will be read-only operations
    // for now everything requires write access so it's a waste
    pub player_manager: SyncMutex<PlayerManager>,
    inner: RwLock<ServerStateInner>,
}

impl ServerState {
    pub fn new(http_client: reqwest::Client, central_url: String, central_pw: String) -> Self {
        Self {
            player_count: AtomicU32::new(0u32),
            player_manager: SyncMutex::new(PlayerManager::new()),
            inner: RwLock::new(ServerStateInner {
                http_client,
                central_url,
                central_pw,
            }),
        }
    }

    pub async fn read(&self) -> RwLockReadGuard<'_, ServerStateInner> {
        self.inner.read().await
    }

    pub async fn write(&self) -> RwLockWriteGuard<'_, ServerStateInner> {
        self.inner.write().await
    }
}
