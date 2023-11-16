use std::sync::{atomic::AtomicU32, Arc};
use tokio::sync::RwLock;

pub struct ServerStateData {
    pub http_client: reqwest::Client,
    pub central_url: String,
    pub central_pw: String,
    pub player_count: AtomicU32,
}

pub type ServerState = Arc<RwLock<ServerStateData>>;
