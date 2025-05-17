use std::{fmt::Display, time::Duration};

use globed_shared::reqwest;
use serde::Deserialize;

struct ClientConfig {
    base_url: String,
    api_token: Option<String>,
}

impl ClientConfig {
    pub fn new(mut base_url: String, api_token: Option<String>) -> Self {
        while base_url.ends_with('/') {
            base_url.pop();
        }

        Self { base_url, api_token }
    }
}

// todo: if this ever is gonna store internal state then modify the thing in main
pub struct ArgonClient {
    config: ClientConfig,
    client: reqwest::Client,
}

pub enum Verdict {
    Strong,
    Weak(String),
    Invalid(String),
}

pub enum ArgonClientError {
    RequestFailed(reqwest::Error),
    InvalidJson(serde_json::Error),
    ArgonError(String),
}

impl From<reqwest::Error> for ArgonClientError {
    fn from(value: reqwest::Error) -> Self {
        Self::RequestFailed(value)
    }
}

impl From<serde_json::Error> for ArgonClientError {
    fn from(value: serde_json::Error) -> Self {
        Self::InvalidJson(value)
    }
}

impl Display for ArgonClientError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::RequestFailed(err) => write!(f, "request failed: {err}"),
            Self::InvalidJson(msg) => write!(f, "invalid server response: {msg}"),
            Self::ArgonError(msg) => write!(f, "error from the server: {msg}"),
        }
    }
}

#[derive(Deserialize)]
pub struct ArgonStatus {
    pub active: bool,
    pub total_nodes: usize,
    pub active_nodes: usize,
    pub ident: String,
}

#[derive(Deserialize)]
struct ArgonResponse {
    pub valid: bool,
    pub valid_weak: bool,
    #[serde(default)]
    pub cause: Option<String>,
    #[serde(default)]
    pub username: Option<String>,
}

impl ArgonClient {
    pub fn new(base_url: String, api_token: Option<String>) -> Self {
        let invalid_certs = std::env::var("GLOBED_ALLOW_INVALID_CERTS").is_ok_and(|x| x != "0");
        let no_proxy = std::env::var("GLOBED_NO_ARGON_SYSPROXY").is_ok_and(|x| x != "0");

        let mut http_client = reqwest::ClientBuilder::new()
            .use_rustls_tls()
            .danger_accept_invalid_certs(invalid_certs)
            .user_agent(format!("globed-central-server/{}", env!("CARGO_PKG_VERSION")))
            .timeout(Duration::from_secs(10));

        if no_proxy {
            http_client = http_client.no_proxy();
        }

        let http_client = http_client.build().unwrap();

        let config = ClientConfig::new(base_url, api_token);

        Self { client: http_client, config }
    }

    pub async fn validate_token(&self, account_id: i32, user_id: i32, username: &str, token: &str) -> Result<Verdict, ArgonClientError> {
        let mut req = self.client.get(format!("{}/v1/validation/check_strong", self.config.base_url)).query(&[
            ("account_id", account_id.to_string().as_str()),
            ("user_id", user_id.to_string().as_str()),
            ("username", username),
            ("authtoken", token),
        ]);

        if let Some(token) = self.config.api_token.as_ref() {
            req = req.bearer_auth(token);
        }

        let response = req.send().await?;

        if !response.status().is_success() {
            let resp = response.text().await?;
            return Err(ArgonClientError::ArgonError(resp));
        }

        let response: ArgonResponse = match serde_json::from_str(&response.text().await?) {
            Ok(x) => x,
            Err(e) => return Err(ArgonClientError::InvalidJson(e)),
        };

        if !response.valid_weak {
            Ok(Verdict::Invalid(response.cause.unwrap_or_else(|| "unknown".to_owned())))
        } else if !response.valid {
            Ok(Verdict::Weak(response.username.unwrap_or_else(|| "<unknown>".to_owned())))
        } else {
            Ok(Verdict::Strong)
        }
    }

    pub async fn check_status(&self) -> Result<ArgonStatus, ArgonClientError> {
        let req = self.client.get(format!("{}/v1/status", self.config.base_url));

        let response = req.send().await?;

        if !response.status().is_success() {
            let resp = response.text().await?;
            return Err(ArgonClientError::ArgonError(resp));
        }

        Ok(serde_json::from_str(&response.text().await?)?)
    }
}
