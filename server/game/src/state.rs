use std::sync::Arc;
use tokio::sync::RwLock;

pub struct ServerStateData {
    pub http_client: reqwest::Client,
    pub central_url: String,
    pub central_pw: String,
}

pub type ServerState = Arc<RwLock<ServerStateData>>;
