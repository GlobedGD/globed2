use std::{
    error::Error,
    fmt::Display,
    sync::atomic::{AtomicBool, Ordering},
    time::Duration,
};

use esp::{size_of_types, ByteBufferExtRead, ByteBufferExtWrite, ByteReader, DecodeError, FastByteBuffer, StaticSize};
use globed_shared::{
    reqwest::{self, StatusCode},
    GameServerBootData, SyncMutex, TokenIssuer, UserEntry, PROTOCOL_VERSION, SERVER_MAGIC, SERVER_MAGIC_LEN,
};

use crate::server_thread::handlers::admin::*;

use serde::Serialize;

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
    pub webhook_present: AtomicBool,
}

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
                "protocol mismatch, we are on v{PROTOCOL_VERSION} while central server is on v{proto}"
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
            webhook_present: AtomicBool::new(false),
        }
    }

    pub fn is_maintenance(&self) -> bool {
        self.maintenance.load(Ordering::Relaxed)
    }

    pub fn is_whitelist(&self) -> bool {
        self.whitelist.load(Ordering::Relaxed)
    }

    pub fn has_webhook(&self) -> bool {
        self.webhook_present.load(Ordering::Relaxed)
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

        if boot_data.protocol != PROTOCOL_VERSION {
            return Err(CentralBridgeError::ProtocolMismatch(boot_data.protocol));
        }

        Ok(boot_data)
    }

    pub async fn refresh_boot_data(&self) -> Result<()> {
        let data = self.request_boot_data().await?;

        // update various values
        self.token_issuer
            .lock()
            .set_expiration_period(Duration::from_secs(data.token_expiry));

        // set the data
        self.set_boot_data(data);

        Ok(())
    }

    #[inline]
    pub fn set_boot_data(&self, data: GameServerBootData) {
        self.maintenance.store(data.maintenance, Ordering::Relaxed);
        self.whitelist.store(data.whitelist, Ordering::Relaxed);
        self.webhook_present
            .store(!data.admin_webhook_url.is_empty(), Ordering::Relaxed);

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

        Ok(reader.read_value::<UserEntry>()?)
    }

    pub async fn update_user_data(&self, user: &UserEntry) -> Result<()> {
        let mut rawbuf = [0u8; UserEntry::ENCODED_SIZE + size_of_types!(u32)];
        let mut buffer = FastByteBuffer::new(&mut rawbuf);

        buffer.write_value(user);
        buffer.append_self_checksum();

        let body = buffer.to_vec();

        let response = self
            .http_client
            .post(format!("{}gs/user/update", self.central_url))
            .header("Authorization", self.central_pw.clone())
            .body(body)
            .send()
            .await?;

        let status = response.status();
        if !status.is_success() {
            let message = response.text().await.unwrap_or_else(|_| "<no response>".to_owned());

            return Err(CentralBridgeError::CentralError((status, message)));
        }

        Ok(())
    }

    fn role_to_string(role: i32) -> String {
        match role {
            ROLE_USER => "User",
            ROLE_HELPER => "Helper",
            ROLE_MOD => "Moderator",
            ROLE_ADMIN => "Admin",
            _ => "Unknown",
        }
        .to_owned()
    }

    #[allow(clippy::too_many_lines)]
    fn embed_for_message(message: &WebhookMessage) -> Option<WebhookEmbed> {
        match message {
            WebhookMessage::AuthFail(_user) => None,
            WebhookMessage::NoticeToEveryone(username, _player_count, message) => Some(WebhookEmbed {
                title: "Global notice".to_owned(),
                color: hex_color_to_decimal("#4dace8"),
                author: Some(WebhookAuthor {
                    name: username,
                    icon_url: None,
                }),
                description: Some(message.clone()),
                footer: None,
                fields: Vec::new(),
            }),
            WebhookMessage::NoticeToSelection(username, player_count, message) => Some(WebhookEmbed {
                title: "Notice".to_owned(),
                color: hex_color_to_decimal("#4dace8"),
                author: Some(WebhookAuthor {
                    name: username,
                    icon_url: None,
                }),
                description: Some(message.clone()),
                footer: None,
                fields: vec![WebhookField {
                    name: "Sent to",
                    value: format!("{player_count} people"),
                    inline: Some(true),
                }],
            }),
            WebhookMessage::NoticeToPerson(author, target, message) => Some(WebhookEmbed {
                title: format!("Notice for {target}"),
                color: hex_color_to_decimal("#4dace8"),
                author: Some(WebhookAuthor {
                    name: author,
                    icon_url: None,
                }),
                description: Some(message.clone()),
                footer: None,
                fields: Vec::new(),
            }),
            WebhookMessage::KickEveryone(username, reason) => Some(WebhookEmbed {
                title: "Kick everyone".to_owned(),
                color: hex_color_to_decimal("#e8d34d"),
                author: Some(WebhookAuthor {
                    name: username,
                    icon_url: None,
                }),
                description: Some(reason.clone()),
                footer: None,
                fields: Vec::new(),
            }),
            WebhookMessage::KickPerson(username, target_username, target_id, reason) => Some(WebhookEmbed {
                title: format!("Kick {target_username}"),
                color: hex_color_to_decimal("#e8d34d"),
                author: Some(WebhookAuthor {
                    name: username,
                    icon_url: None,
                }),
                description: Some(reason.clone()),
                footer: None,
                fields: vec![WebhookField {
                    name: "Account ID",
                    value: target_id.to_string(),
                    inline: Some(true),
                }],
            }),
            WebhookMessage::UserBanChanged(bmsc) => Some(WebhookEmbed {
                title: if bmsc.new_state {
                    "User banned".to_owned()
                } else {
                    "User unbanned".to_owned()
                },
                color: hex_color_to_decimal("#de3023"),
                author: Some(WebhookAuthor {
                    name: &bmsc.mod_name,
                    icon_url: None,
                }),
                description: if bmsc.new_state {
                    Some(bmsc.reason.clone().unwrap_or_else(|| "No reason given.".to_string()))
                } else {
                    None
                },
                footer: None,
                fields: if bmsc.new_state {
                    vec![WebhookField {
                        name: "Expires",
                        value: if let Some(seconds) = bmsc.expiry {
                            format!("<t:{seconds}:f>")
                        } else {
                            "Permanent.".to_owned()
                        },
                        inline: Some(true),
                    }]
                } else {
                    vec![]
                },
            }),
            WebhookMessage::UserMuteChanged(bmsc) => Some(WebhookEmbed {
                title: if bmsc.new_state {
                    "User muted".to_owned()
                } else {
                    "User unmuted".to_owned()
                },
                color: hex_color_to_decimal("#ded823"),
                author: Some(WebhookAuthor {
                    name: &bmsc.mod_name,
                    icon_url: None,
                }),
                description: if bmsc.new_state {
                    Some(bmsc.reason.clone().unwrap_or_else(|| "No reason given.".to_string()))
                } else {
                    None
                },
                footer: None,
                fields: if bmsc.new_state {
                    vec![WebhookField {
                        name: "Expires",
                        value: if let Some(seconds) = bmsc.expiry {
                            format!("<t:{seconds}:f>")
                        } else {
                            "Permanent.".to_owned()
                        },
                        inline: Some(true),
                    }]
                } else {
                    vec![]
                },
            }),
            WebhookMessage::UserViolationMetaChanged(mod_name, user_name, expiry, reason) => Some(WebhookEmbed {
                title: "Ban/mute state changed".to_owned(),
                color: hex_color_to_decimal("#de7a23"),
                author: Some(WebhookAuthor {
                    name: mod_name,
                    icon_url: None,
                }),
                description: None,
                footer: None,
                fields: vec![
                    WebhookField {
                        name: "Username",
                        value: user_name.clone(),
                        inline: Some(false),
                    },
                    WebhookField {
                        name: "Reason",
                        value: reason.clone().unwrap_or_else(|| "No reason given.".to_owned()),
                        inline: Some(false),
                    },
                    WebhookField {
                        name: "Expiration",
                        value: expiry
                            .map(|x| format!("<t:{x}:f>"))
                            .unwrap_or_else(|| "Permanent.".to_owned()),
                        inline: Some(false),
                    },
                ],
            }),
            WebhookMessage::UserRoleChanged(mod_name, user_name, old_role, new_role) => Some(WebhookEmbed {
                title: "Role change".to_owned(),
                color: hex_color_to_decimal("#8b4de8"),
                author: Some(WebhookAuthor {
                    name: mod_name,
                    icon_url: None,
                }),
                description: None,
                footer: None,
                fields: vec![
                    WebhookField {
                        name: "Username",
                        value: user_name.clone(),
                        inline: Some(true),
                    },
                    WebhookField {
                        name: "Old role",
                        value: Self::role_to_string(*old_role),
                        inline: Some(true),
                    },
                    WebhookField {
                        name: "New role",
                        value: Self::role_to_string(*new_role),
                        inline: Some(true),
                    },
                ],
            }),
            WebhookMessage::UserNameColorChanged(mod_name, user_name, old_color, new_color) => Some(WebhookEmbed {
                title: "Name color change".to_owned(),
                color: hex_color_to_decimal(new_color.as_ref().map_or_else(|| "", |x| x.as_str())),
                author: Some(WebhookAuthor {
                    name: mod_name,
                    icon_url: None,
                }),
                description: None,
                footer: None,
                fields: vec![
                    WebhookField {
                        name: "Username",
                        value: user_name.clone(),
                        inline: Some(true),
                    },
                    WebhookField {
                        name: "Old color",
                        value: old_color.clone().unwrap_or_else(|| "none".to_owned()),
                        inline: Some(true),
                    },
                    WebhookField {
                        name: "New color",
                        value: new_color.clone().unwrap_or_else(|| "none".to_owned()),
                        inline: Some(true),
                    },
                ],
            }),
        }
    }

    #[inline]
    pub async fn send_webhook_message(&self, message: WebhookMessage) -> Result<()> {
        let messages = [message];
        self.send_webhook_messages(&messages).await
    }

    // not really bridge but it was making web requests which is sorta related i guess
    pub async fn send_webhook_messages(&self, messages: &[WebhookMessage]) -> Result<()> {
        let url = self.central_conf.lock().admin_webhook_url.clone();

        let mut content = String::new();
        let mut embeds = Vec::new();

        for message in messages {
            match message {
                WebhookMessage::AuthFail(username) => {
                    content += &format!("{username} just tried to login to the admin panel and failed.");
                }
                msg => {
                    if let Some(embed) = Self::embed_for_message(msg) {
                        embeds.push(embed);
                    }
                }
            }
        }

        let opts = WebhookOpts {
            username: "in-game actions",
            content: if content.is_empty() { None } else { Some(&content) },
            embeds,
        };

        let response = self
            .http_client
            .post(url)
            .header("Content-Type", "application/json")
            .body(serde_json::to_string(&opts).map_err(|e| CentralBridgeError::Other(e.to_string()))?)
            .send()
            .await?;

        let status = response.status();
        if !status.is_success() {
            let message = response.text().await.unwrap_or_else(|_| "<no response>".to_owned());

            return Err(CentralBridgeError::WebhookError((status, message)));
        }

        Ok(())
    }
}

pub struct BanMuteStateChange {
    pub mod_name: String,
    pub target_name: String,
    pub target_id: i32,
    pub new_state: bool,
    pub expiry: Option<i64>,
    pub reason: Option<String>,
}

pub enum WebhookMessage {
    AuthFail(String),                                                      // username
    NoticeToEveryone(String, usize, String),                               // username, player count, message
    NoticeToSelection(String, usize, String),                              // username, player count, message
    NoticeToPerson(String, String, String),                                // author, target username, message
    KickEveryone(String, String),                                          // mod username, reason
    KickPerson(String, String, i32, String), // mod username, target username, target account id, reason
    UserBanChanged(BanMuteStateChange),      // yeah
    UserMuteChanged(BanMuteStateChange),     // yeah
    UserViolationMetaChanged(String, String, Option<i64>, Option<String>), // mod username, username, expiry, reason
    UserRoleChanged(String, String, i32, i32), // mod username, username, old role, new role
    UserNameColorChanged(String, String, Option<String>, Option<String>), // mod username, username, old color, new color
}

#[derive(Serialize)]
struct WebhookAuthor<'a> {
    name: &'a str,
    #[serde(skip_serializing_if = "Option::is_none")]
    icon_url: Option<&'a str>,
}

#[derive(Serialize)]
struct WebhookFooter<'a> {
    pub text: &'a str,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub icon_url: Option<&'a str>,
}

#[derive(Serialize)]
struct WebhookField<'a> {
    name: &'a str,
    value: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    inline: Option<bool>,
}

#[derive(Serialize)]
struct WebhookEmbed<'a> {
    pub title: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub color: Option<u32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub author: Option<WebhookAuthor<'a>>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub description: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub footer: Option<WebhookFooter<'a>>,
    #[serde(skip_serializing_if = "Vec::is_empty")]
    pub fields: Vec<WebhookField<'a>>,
}

#[derive(Serialize)]
struct WebhookOpts<'a> {
    username: &'a str,

    #[serde(skip_serializing_if = "Option::is_none")]
    content: Option<&'a str>,

    #[serde(skip_serializing_if = "Vec::is_empty")]
    embeds: Vec<WebhookEmbed<'a>>,
}

fn hex_color_to_decimal(color: &str) -> Option<u32> {
    let color = color.strip_prefix('#').unwrap_or(color);

    u32::from_str_radix(color, 16).ok()
}
