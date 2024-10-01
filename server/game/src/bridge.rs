use std::{
    error::Error,
    fmt::Display,
    sync::atomic::{AtomicBool, Ordering},
    time::Duration,
};

use esp::{
    size_of_types, ByteBuffer, ByteBufferExt, ByteBufferExtRead, ByteBufferExtWrite, ByteReader, Decodable, DecodeError, DynamicSize, Encodable,
    FastVec, StaticSize,
};
use globed_derive::{DynamicSize, Encodable};
use globed_shared::{
    data::*,
    reqwest::{self, Response, StatusCode},
    GameServerBootData, ServerUserEntry, SyncMutex, TokenIssuer, UserLoginResponse, MAX_SUPPORTED_PROTOCOL, SERVER_MAGIC, SERVER_MAGIC_LEN,
};

use crate::webhook::{self, *};

#[derive(Debug)]
pub enum CentralBridgeError {
    RequestError(reqwest::Error),       // error making the request
    CentralError((StatusCode, String)), // non 2xx status code
    WebhookError((StatusCode, String)), // non 2xx status code
    InvalidMagic(String),               // invalid magic
    MalformedData(DecodeError),         // failed to decode data
    ProtocolMismatch(u16),              // protocol version mismatch
    Other(String),
}

impl Display for CentralBridgeError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::RequestError(err) => write!(f, "error making web request: {err}"),
            Self::CentralError((err, response)) => write!(f, "central error {err}: {response}"),
            Self::WebhookError((err, response)) => write!(f, "webhook error {err}: {response}"),
            Self::InvalidMagic(_) => write!(f, "central server sent invalid magic"),
            Self::MalformedData(err) => write!(f, "failed to decode data sent by the central server: {err}"),
            Self::ProtocolMismatch(proto) => write!(
                f,
                "protocol mismatch, we are on v{MAX_SUPPORTED_PROTOCOL} while central server is on v{proto}"
            ),
            Self::Other(err) => f.write_str(err),
        }
    }
}

impl From<reqwest::Error> for CentralBridgeError {
    fn from(value: reqwest::Error) -> Self {
        Self::RequestError(value)
    }
}

impl From<DecodeError> for CentralBridgeError {
    fn from(value: DecodeError) -> Self {
        Self::MalformedData(value)
    }
}

impl Error for CentralBridgeError {}

pub type Result<T> = std::result::Result<T, CentralBridgeError>;

/// `CentralBridge` stores the configuration of the game server,
/// and is used for making requests to the central server.
pub struct CentralBridge {
    pub http_client: reqwest::Client,
    pub central_url: String,
    pub central_pw: String,
    pub token_issuer: SyncMutex<TokenIssuer>,
    pub central_conf: SyncMutex<GameServerBootData>,

    // for performance reasons /shrug
    pub maintenance: AtomicBool,
    pub whitelist: AtomicBool,
    pub admin_webhook_present: AtomicBool,
    pub featured_webhook_present: AtomicBool,
    pub room_webhook_present: AtomicBool,
}

#[derive(Encodable, DynamicSize)]
struct UserLoginPayload<'a> {
    pub account_id: i32,
    pub username: &'a str,
}

pub enum AdminUserAction {
    UpdateUsername(AdminUpdateUsernameAction),
    SetNameColor(AdminSetNameColorAction),
    SetUserRoles(AdminSetUserRolesAction),
    PunishUser(AdminPunishUserAction),
    RemovePunishment(AdminRemovePunishmentAction),
    Whitelist(AdminWhitelistAction),
    SetAdminPassword(AdminSetAdminPasswordAction),
    EditPunishment(AdminEditPunishmentAction),
}

impl CentralBridge {
    pub fn new(central_url: &str, central_pw: &str) -> Self {
        let http_client = reqwest::Client::builder()
            .timeout(Duration::from_secs(5))
            .user_agent(format!("globed-game-server/{}", env!("CARGO_PKG_VERSION")))
            .build()
            .unwrap();

        Self {
            http_client,
            token_issuer: SyncMutex::new(TokenIssuer::new("", Duration::from_secs(0))),
            central_url: central_url.to_owned(),
            central_pw: central_pw.to_owned(),
            central_conf: SyncMutex::new(GameServerBootData::default()),
            maintenance: AtomicBool::new(false),
            whitelist: AtomicBool::new(false),
            admin_webhook_present: AtomicBool::new(false),
            featured_webhook_present: AtomicBool::new(false),
            room_webhook_present: AtomicBool::new(false),
        }
    }

    pub fn is_maintenance(&self) -> bool {
        self.maintenance.load(Ordering::Relaxed)
    }

    pub fn is_whitelist(&self) -> bool {
        self.whitelist.load(Ordering::Relaxed)
    }

    pub fn has_admin_webhook(&self) -> bool {
        self.admin_webhook_present.load(Ordering::Relaxed)
    }

    pub fn has_featured_webhook(&self) -> bool {
        self.featured_webhook_present.load(Ordering::Relaxed)
    }

    pub fn has_room_webhook(&self) -> bool {
        self.room_webhook_present.load(Ordering::Relaxed)
    }

    pub async fn request_boot_data(&self) -> Result<GameServerBootData> {
        let response = self
            .http_client
            .post(format!("{}gs/boot", self.central_url))
            .header("Authorization", self.central_pw.clone())
            .send()
            .await?;

        let status = response.status();
        if !status.is_success() {
            let message = response.text().await.unwrap_or_else(|_| "<no response>".to_owned());

            return Err(CentralBridgeError::CentralError((status, message)));
        }

        let config = response.bytes().await?;
        let mut reader = ByteReader::from_bytes(&config);

        // verify that the magic bytes match
        let valid_magic = reader
            .read_value_array::<u8, SERVER_MAGIC_LEN>()
            .map_or(false, |magic| magic.iter().eq(SERVER_MAGIC.iter()));

        if !valid_magic {
            let txt = String::from_utf8(reader.as_bytes().to_vec()).unwrap_or_else(|_| "<invalid UTF-8 string>".to_owned());
            return Err(CentralBridgeError::InvalidMagic(txt));
        }

        let boot_data = reader.read_value::<GameServerBootData>()?;

        if boot_data.protocol != MAX_SUPPORTED_PROTOCOL {
            return Err(CentralBridgeError::ProtocolMismatch(boot_data.protocol));
        }

        Ok(boot_data)
    }

    pub async fn refresh_boot_data(&self) -> Result<()> {
        let data = self.request_boot_data().await?;

        // update various values
        self.token_issuer.lock().set_expiration_period(Duration::from_secs(data.token_expiry));

        // set the data
        self.set_boot_data(data);

        Ok(())
    }

    #[inline]
    pub fn set_boot_data(&self, data: GameServerBootData) {
        self.maintenance.store(data.maintenance, Ordering::Relaxed);
        self.whitelist.store(data.whitelist, Ordering::Relaxed);
        self.admin_webhook_present.store(!data.admin_webhook_url.is_empty(), Ordering::Relaxed);
        self.featured_webhook_present
            .store(!data.featured_webhook_url.is_empty(), Ordering::Relaxed);
        self.room_webhook_present.store(!data.room_webhook_url.is_empty(), Ordering::Relaxed);

        let mut issuer = self.token_issuer.lock();

        issuer.set_expiration_period(Duration::from_secs(data.token_expiry));
        issuer.set_secret_key(&data.secret_key2);

        *self.central_conf.lock() = data;
    }

    // other web requests
    pub async fn get_user_data(&self, player: &str) -> Result<UserEntry> {
        let response = self
            .http_client
            .get(format!("{}gs/user/{}", self.central_url, player))
            .header("Authorization", self.central_pw.clone())
            .send()
            .await?;

        let status = response.status();
        if !status.is_success() {
            let message = response.text().await.unwrap_or_else(|_| "<no response>".to_owned());

            return Err(CentralBridgeError::CentralError((status, message)));
        }

        let config = response.bytes().await?;
        let mut reader = ByteReader::from_bytes(&config);
        reader.validate_self_checksum()?;

        Ok(reader.read_value()?)
    }

    pub async fn user_login(&self, account_id: i32, username: &str) -> Result<UserLoginResponse> {
        let payload = UserLoginPayload { account_id, username };

        self._send_encoded_body_req_resp("gs/userlogin", &payload).await
    }

    pub async fn send_admin_user_action(
        &self,
        action: &AdminUserAction,
    ) -> Result<(ServerUserEntry, Option<UserPunishment>, Option<UserPunishment>)> {
        match action {
            AdminUserAction::UpdateUsername(x) => Ok((self._send_encoded_body_req_resp("user/update/username", x).await?, None, None)),
            AdminUserAction::SetNameColor(x) => Ok((self._send_encoded_body_req_resp("user/update/name_color", x).await?, None, None)),
            AdminUserAction::SetUserRoles(x) => Ok((self._send_encoded_body_req_resp("user/update/roles", x).await?, None, None)),
            AdminUserAction::PunishUser(x) => self._send_encoded_body_req_resp("user/update/punish", x).await,
            AdminUserAction::RemovePunishment(x) => self._send_encoded_body_req_resp("user/update/unpunish", x).await,
            AdminUserAction::Whitelist(x) => Ok((self._send_encoded_body_req_resp("user/update/whitelist", x).await?, None, None)),
            AdminUserAction::SetAdminPassword(x) => Ok((self._send_encoded_body_req_resp("user/update/adminpw", x).await?, None, None)),
            AdminUserAction::EditPunishment(x) => self._send_encoded_body_req_resp("user/update/editpunish", x).await,
        }
    }

    pub async fn get_punishment_history(&self, account_id: i32) -> Result<Vec<UserPunishment>> {
        let response = self
            .http_client
            .get(format!("{}user/punishment_history", self.central_url))
            .query(&[("account_id", account_id)])
            .header("Authorization", self.central_pw.clone())
            .send()
            .await?;

        let status = response.status();
        if !status.is_success() {
            let message = response.text().await.unwrap_or_else(|_| "<no response>".to_owned());

            return Err(CentralBridgeError::CentralError((status, message)));
        }

        let data = response.bytes().await?;
        let mut reader = ByteReader::from_bytes(&data);
        reader.validate_self_checksum()?;

        Ok(reader.read_value()?)
    }

    #[inline]
    pub async fn send_admin_webhook_message(&self, message: WebhookMessage) -> Result<()> {
        self.send_webhook_messages(&[message], WebhookChannel::Admin).await
    }

    #[inline]
    pub async fn send_rate_suggestion_webhook_message(&self, message: WebhookMessage) -> Result<()> {
        self.send_webhook_messages(&[message], WebhookChannel::RateSuggestion).await
    }

    async fn _send_encoded_body_req<T: Encodable + DynamicSize>(&self, url_suffix: &str, body: &T) -> Result<Response> {
        let mut buffer = ByteBuffer::with_capacity(body.encoded_size() + size_of_types!(u32));

        buffer.write_value(body);
        buffer.append_self_checksum();

        let body = buffer.into_vec();

        let response = self
            .http_client
            .post(format!("{}{}", self.central_url, url_suffix))
            .header("Authorization", self.central_pw.clone())
            .body(body)
            .send()
            .await?;

        let status = response.status();
        if !status.is_success() {
            let message = response.text().await.unwrap_or_else(|_| "<no response>".to_owned());

            return Err(CentralBridgeError::CentralError((status, message)));
        }

        Ok(response)
    }

    async fn _send_encoded_body_req_resp<R: Decodable, T: Encodable + DynamicSize>(&self, url_suffix: &str, body: &T) -> Result<R> {
        let response = self._send_encoded_body_req::<T>(url_suffix, body).await?;

        let data = response.bytes().await?;
        let mut reader = ByteReader::from_bytes(&data);
        reader.validate_self_checksum()?;

        Ok(reader.read_value::<R>()?)
    }

    // not really bridge but it was making web requests which is sorta related i guess
    pub async fn send_webhook_messages(&self, messages: &[WebhookMessage], channel: WebhookChannel) -> Result<()> {
        let url = match channel {
            WebhookChannel::Admin => self.central_conf.lock().admin_webhook_url.clone(),
            WebhookChannel::RateSuggestion => self.central_conf.lock().rate_suggestion_webhook_url.clone(),
            WebhookChannel::Featured => self.central_conf.lock().featured_webhook_url.clone(),
            WebhookChannel::Room => self.central_conf.lock().room_webhook_url.clone(),
        };

        webhook::send_webhook_messages(self.http_client.clone(), &url, messages, None)
            .await
            .map_err(|e| match e {
                WebhookError::Serialization(e) => CentralBridgeError::Other(e),
                WebhookError::Request(e) => CentralBridgeError::RequestError(e),
                WebhookError::RequestStatus((sc, message)) => CentralBridgeError::WebhookError((sc, message)),
            })?;

        Ok(())
    }

    pub async fn send_webhook_message_for_action(
        &self,
        action: &AdminUserAction,
        user: &ServerUserEntry,
        _mod_id: i32,
        mod_name: String,
        ban: Option<UserPunishment>,
        mute: Option<UserPunishment>,
    ) -> Result<()> {
        let mut messages = FastVec::<WebhookMessage, 4>::new();

        match action {
            AdminUserAction::UpdateUsername(_) => {}
            AdminUserAction::SetNameColor(_act) => {
                messages.push(WebhookMessage::UserNameColorChanged(UserNameColorChange {
                    account_id: user.account_id,
                    mod_name,
                    name: user.user_name.clone().unwrap_or_default(),
                    new_color: user.name_color.clone(),
                }));
            }
            AdminUserAction::SetUserRoles(_act) => {
                messages.push(WebhookMessage::UserRolesChanged(
                    mod_name,
                    user.user_name.clone().unwrap_or_default(),
                    user.user_roles.clone(),
                ));
            }
            AdminUserAction::PunishUser(act) => {
                if act.is_ban {
                    let ban = if let Some(ban) = ban {
                        ban
                    } else {
                        return Err(CentralBridgeError::Other("internal error: ban is None when punishing user".to_owned()));
                    };

                    let bmsc = BanMuteStateChange {
                        mod_name,
                        target_name: user.user_name.clone().unwrap_or_default(),
                        target_id: user.account_id,
                        new_state: true,
                        expiry: if ban.expires_at == 0 { None } else { Some(ban.expires_at) },
                        reason: if ban.reason.is_empty() { None } else { Some(ban.reason) },
                    };

                    messages.push(WebhookMessage::UserBanned(bmsc));
                } else {
                    let mute = if let Some(mute) = mute {
                        mute
                    } else {
                        return Err(CentralBridgeError::Other("internal error: mute is None when punishing user".to_owned()));
                    };

                    let bmsc = BanMuteStateChange {
                        mod_name,
                        target_name: user.user_name.clone().unwrap_or_default(),
                        target_id: user.account_id,
                        new_state: true,
                        expiry: if mute.expires_at == 0 { None } else { Some(mute.expires_at) },
                        reason: if mute.reason.is_empty() { None } else { Some(mute.reason) },
                    };

                    messages.push(WebhookMessage::UserMuted(bmsc));
                }
            }
            AdminUserAction::RemovePunishment(act) => {
                if act.is_ban {
                    messages.push(WebhookMessage::UserUnbanned(PunishmentRemoval {
                        account_id: user.account_id,
                        name: user.user_name.clone().unwrap_or_default(),
                        mod_name,
                    }));
                } else {
                    messages.push(WebhookMessage::UserUnmuted(PunishmentRemoval {
                        account_id: user.account_id,
                        name: user.user_name.clone().unwrap_or_default(),
                        mod_name,
                    }));
                }
            }
            AdminUserAction::Whitelist(_action) => {
                // messages.push(WebhookMessage::Wh);
                // TODO
            }
            AdminUserAction::SetAdminPassword(_) => {}
            AdminUserAction::EditPunishment(action) => {
                messages.push(WebhookMessage::UserViolationMetaChanged(ViolationMetaChange {
                    account_id: user.account_id,
                    name: user.user_name.clone().unwrap_or_default(),
                    is_ban: action.is_ban,
                    expiry: if action.expires_at == 0 { None } else { Some(action.expires_at) },
                    reason: if action.reason.is_empty() {
                        None
                    } else {
                        Some(action.reason.to_string())
                    },
                    mod_name,
                }));
            }
        };

        if messages.is_empty() {
            return Ok(());
        }

        self.send_webhook_messages(&messages, WebhookChannel::Admin).await
    }
}
