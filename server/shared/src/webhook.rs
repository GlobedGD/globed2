use reqwest::StatusCode;
use serde::Serialize;

pub struct BanMuteStateChange {
    pub mod_name: String,
    pub target_name: String,
    pub target_id: i32,
    pub new_state: bool,
    pub expiry: Option<i64>,
    pub reason: Option<String>,
}

pub struct PunishmentRemoval {
    pub account_id: i32,
    pub name: String,
    pub mod_name: String,
}

pub struct UserNameColorChange {
    pub account_id: i32,
    pub name: String,
    pub new_color: Option<String>,
    pub mod_name: String,
}

pub struct ViolationMetaChange {
    pub account_id: i32,
    pub name: String,
    pub is_ban: bool,
    pub expiry: Option<u64>,
    pub reason: Option<String>,
    pub mod_name: String,
}

pub enum WebhookMessage {
    AuthFail(String),                         // username
    NoticeToEveryone(String, usize, String),  // username, player count, message
    NoticeToSelection(String, usize, String), // username, player count, message
    NoticeToPerson(String, String, String),   // author, target username, message
    KickEveryone(String, String),             // mod username, reason
    KickPerson(String, String, i32, String),  // mod username, target username, target account id, reason
    UserBanned(BanMuteStateChange),
    UserUnbanned(PunishmentRemoval),
    UserMuted(BanMuteStateChange),
    UserUnmuted(PunishmentRemoval),
    UserViolationMetaChanged(ViolationMetaChange), // mod username, username, is_banned, is_muted, expiry, reason
    UserRolesChanged(String, String, Vec<String>), // mod username, username, new roles
    UserNameColorChanged(UserNameColorChange),
    FeaturedLevelSend(i32, String, String, i32, String, i32, i32, Option<String>), // user id, user name, level name, level id, level author, difficulty, rate tier, notes
    LevelFeatured(String, i32, String, i32, i32),                                  // level name, level id, level author, difficulty, rate tier
    RoomCreated(u32, String, String, i32, bool, bool),                             // room id, room name, username, account id, hidden, protected
}

#[derive(Debug)]
pub enum WebhookChannel {
    Admin,
    Featured,
    RateSuggestion,
    Room,
}

#[derive(Serialize)]
pub struct WebhookAuthor {
    pub name: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub icon_url: Option<String>,
}

#[derive(Serialize)]
pub struct WebhookFooter<'a> {
    pub text: &'a str,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub icon_url: Option<&'a str>,
}

#[derive(Serialize)]
pub struct WebhookField<'a> {
    pub name: &'a str,
    pub value: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub inline: Option<bool>,
}

#[derive(Serialize)]
pub struct WebhookThumbnail {
    pub url: String,
}

#[derive(Default, Serialize)]
pub struct WebhookEmbed<'a> {
    pub title: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub color: Option<u32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub author: Option<WebhookAuthor>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub description: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub footer: Option<WebhookFooter<'a>>,
    #[serde(skip_serializing_if = "Vec::is_empty")]
    pub fields: Vec<WebhookField<'a>>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub thumbnail: Option<WebhookThumbnail>,
}

#[derive(Serialize)]
pub struct WebhookOpts<'a> {
    #[serde(skip_serializing_if = "Option::is_none")]
    pub username: Option<&'a str>,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub content: Option<String>,

    #[serde(skip_serializing_if = "Vec::is_empty")]
    pub embeds: Vec<WebhookEmbed<'a>>,
}

#[allow(clippy::too_many_lines)]
pub fn embed_for_message(message: &WebhookMessage) -> Option<WebhookEmbed> {
    match message {
        WebhookMessage::AuthFail(_user_name) => None,
        WebhookMessage::NoticeToEveryone(username, player_count, message) => Some(WebhookEmbed {
            title: format!("Global notice (for {player_count} people)"),
            color: hex_color_to_decimal("#4dace8"),
            description: Some(message.clone()),
            fields: vec![WebhookField {
                name: "Performed by",
                value: username.clone(),
                inline: Some(true),
            }],
            ..Default::default()
        }),
        WebhookMessage::NoticeToSelection(username, player_count, message) => Some(WebhookEmbed {
            title: "Notice".to_owned(),
            color: hex_color_to_decimal("#4dace8"),
            description: Some(message.clone()),
            fields: vec![
                WebhookField {
                    name: "Performed by",
                    value: username.clone(),
                    inline: Some(true),
                },
                WebhookField {
                    name: "Sent to",
                    value: format!("{player_count} people"),
                    inline: Some(true),
                },
            ],
            ..Default::default()
        }),
        WebhookMessage::NoticeToPerson(author, target, message) => Some(WebhookEmbed {
            title: format!("Notice for {target}"),
            color: hex_color_to_decimal("#4dace8"),
            author: Some(WebhookAuthor {
                name: target.clone(),
                icon_url: None,
            }),
            description: Some(message.clone()),
            fields: vec![WebhookField {
                name: "Performed by",
                value: author.clone(),
                inline: Some(true),
            }],
            ..Default::default()
        }),
        WebhookMessage::KickEveryone(username, reason) => Some(WebhookEmbed {
            title: "Kick everyone".to_owned(),
            color: hex_color_to_decimal("#e8d34d"),
            description: Some(reason.clone()),
            fields: vec![WebhookField {
                name: "Performed by",
                value: username.clone(),
                inline: Some(true),
            }],
            ..Default::default()
        }),
        WebhookMessage::KickPerson(mod_name, user_name, target_id, reason) => Some(WebhookEmbed {
            title: "Kick user".to_owned(),
            color: hex_color_to_decimal("#e8d34d"),
            author: Some(WebhookAuthor {
                name: format!("{user_name} ({target_id})"),
                icon_url: None,
            }),
            description: Some(reason.clone()),
            fields: vec![WebhookField {
                name: "Performed by",
                value: mod_name.clone(),
                inline: Some(true),
            }],
            ..Default::default()
        }),
        WebhookMessage::UserBanned(bmsc) => Some(WebhookEmbed {
            title: "User banned".to_owned(),
            color: hex_color_to_decimal("#de3023"),
            author: Some(WebhookAuthor {
                name: format!("{} ({})", bmsc.target_name, bmsc.target_id),
                icon_url: None,
            }),
            description: Some(bmsc.reason.clone().unwrap_or_else(|| "No reason given.".to_string())),
            fields: vec![
                WebhookField {
                    name: "Performed by",
                    value: bmsc.mod_name.clone(),
                    inline: Some(true),
                },
                WebhookField {
                    name: "Expires",
                    value: if let Some(seconds) = bmsc.expiry {
                        format!("<t:{seconds}:f>")
                    } else {
                        "Permanent.".to_owned()
                    },
                    inline: Some(true),
                },
            ],
            ..Default::default()
        }),

        WebhookMessage::UserUnbanned(removal) => Some(WebhookEmbed {
            title: "User unbanned".to_owned(),
            color: hex_color_to_decimal("31bd31"),
            author: Some(WebhookAuthor {
                name: format!("{} ({})", removal.name, removal.account_id),
                icon_url: None,
            }),
            description: None,
            fields: vec![WebhookField {
                name: "Performed by",
                value: removal.mod_name.clone(),
                inline: Some(true),
            }],
            ..Default::default()
        }),
        WebhookMessage::UserMuted(bmsc) => Some(WebhookEmbed {
            title: "User muted".to_owned(),
            color: hex_color_to_decimal("#ded823"),
            author: Some(WebhookAuthor {
                name: format!("{} ({})", bmsc.target_name, bmsc.target_id),
                icon_url: None,
            }),
            description: Some(bmsc.reason.clone().unwrap_or_else(|| "No reason given.".to_string())),
            fields: vec![
                WebhookField {
                    name: "Performed by",
                    value: bmsc.mod_name.clone(),
                    inline: Some(true),
                },
                WebhookField {
                    name: "Expires",
                    value: if let Some(seconds) = bmsc.expiry {
                        format!("<t:{seconds}:f>")
                    } else {
                        "Permanent.".to_owned()
                    },
                    inline: Some(true),
                },
            ],
            ..Default::default()
        }),

        WebhookMessage::UserUnmuted(removal) => Some(WebhookEmbed {
            title: "User unmuted".to_owned(),
            color: hex_color_to_decimal("#79bd31"),
            author: Some(WebhookAuthor {
                name: format!("{} ({})", removal.name, removal.account_id),
                icon_url: None,
            }),
            description: None,
            fields: vec![WebhookField {
                name: "Performed by",
                value: removal.mod_name.clone(),
                inline: Some(true),
            }],
            ..Default::default()
        }),
        WebhookMessage::UserViolationMetaChanged(change) => Some(WebhookEmbed {
            title: format!("{} state changed", if change.is_ban { "Ban" } else { "Mute" }),
            color: hex_color_to_decimal("#de7a23"),
            author: Some(WebhookAuthor {
                name: change.name.clone(),
                icon_url: None,
            }),
            fields: vec![
                WebhookField {
                    name: "Performed by",
                    value: change.mod_name.clone(),
                    inline: Some(false),
                },
                WebhookField {
                    name: "Reason",
                    value: change.reason.clone().unwrap_or_else(|| "No reason given.".to_owned()),
                    inline: Some(false),
                },
                WebhookField {
                    name: "Expiration",
                    value: change.expiry.map(|x| format!("<t:{x}:f>")).unwrap_or_else(|| "Permanent.".to_owned()),
                    inline: Some(false),
                },
            ],
            ..Default::default()
        }),
        WebhookMessage::UserRolesChanged(mod_name, user_name, new_roles) => Some(WebhookEmbed {
            title: "Role change".to_owned(),
            color: hex_color_to_decimal("#8b4de8"),
            author: Some(WebhookAuthor {
                name: user_name.clone(),
                icon_url: None,
            }),
            fields: vec![
                WebhookField {
                    name: "Performed by",
                    value: mod_name.clone(),
                    inline: Some(true),
                },
                WebhookField {
                    name: "New roles",
                    value: new_roles.join(", "),
                    inline: Some(true),
                },
            ],
            ..Default::default()
        }),
        WebhookMessage::UserNameColorChanged(change) => Some(WebhookEmbed {
            title: "Name color change".to_owned(),
            color: hex_color_to_decimal(change.new_color.as_ref().map_or_else(|| "", |x| x.as_str())),
            author: Some(WebhookAuthor {
                name: change.name.clone(),
                icon_url: None,
            }),
            fields: vec![
                WebhookField {
                    name: "Performed by",
                    value: change.mod_name.clone(),
                    inline: Some(true),
                },
                WebhookField {
                    name: "New color",
                    value: change.new_color.clone().unwrap_or_else(|| "none".to_owned()),
                    inline: Some(true),
                },
            ],
            ..Default::default()
        }),
        WebhookMessage::FeaturedLevelSend(mod_id, mod_name, level_name, level_id, level_author, difficulty, rate_tier, notes) => Some(WebhookEmbed {
            title: "New level send".to_owned(),
            color: hex_color_to_decimal("#79bd31"),
            author: Some(WebhookAuthor {
                name: format!("{mod_name} ({mod_id})"),
                icon_url: None,
            }),
            fields: vec![
                WebhookField {
                    name: "Level ID",
                    value: level_id.clone().to_string(),
                    inline: Some(true),
                },
                WebhookField {
                    name: "Level Author",
                    value: level_author.clone(),
                    inline: Some(true),
                },
                WebhookField {
                    name: "Level Name",
                    value: level_name.clone(),
                    inline: Some(true),
                },
                WebhookField {
                    name: "Notes",
                    value: notes.clone().unwrap_or_else(|| "None".to_owned()),
                    inline: Some(true),
                },
            ],
            thumbnail: Some(WebhookThumbnail {
                url: rate_tier_to_image(*difficulty, *rate_tier),
            }),
            ..Default::default()
        }),
        WebhookMessage::LevelFeatured(level_name, level_id, level_author, difficulty, rate_tier) => Some(WebhookEmbed {
            title: format!("{level_name} by {level_author}"),
            color: hex_color_to_decimal("#7dfff5"),
            author: Some(WebhookAuthor {
                name: "New Featured Level".to_owned(),
                icon_url: None,
            }),
            fields: vec![WebhookField {
                name: "Level ID",
                value: level_id.clone().to_string(),
                inline: Some(true),
            }],
            thumbnail: Some(WebhookThumbnail {
                url: rate_tier_to_image(*difficulty, *rate_tier),
            }),
            ..Default::default()
        }),
        WebhookMessage::RoomCreated(room_id, room_name, username, account_id, hidden, protected) => Some(WebhookEmbed {
            title: room_name.clone(),
            color: hex_color_to_decimal("#4dace8"),
            author: Some(WebhookAuthor {
                name: format!("{username} ({account_id})"),
                icon_url: None,
            }),
            fields: vec![
                WebhookField {
                    name: "Hidden",
                    value: if *hidden { "Yes" } else { "No" }.to_owned(),
                    inline: Some(true),
                },
                WebhookField {
                    name: "Private",
                    value: if *protected { "Yes" } else { "No" }.to_owned(),
                    inline: Some(true),
                },
                WebhookField {
                    name: "Room ID",
                    value: room_id.to_string(),
                    inline: Some(true),
                },
            ],
            ..Default::default()
        }),
    }
}

pub fn hex_color_to_decimal(color: &str) -> Option<u32> {
    let color = color.strip_prefix('#').unwrap_or(color);

    u32::from_str_radix(color, 16).ok()
}

pub fn rate_tier_to_image(difficulty: i32, tier: i32) -> String {
    let diffname: &str = match difficulty {
        1 => "easy",
        2 => "normal",
        3 => "hard",
        4 => "harder",
        5 => "insane",
        6..=10 => "harddemon",
        _ => "na",
    };

    let ratename: &str = match tier {
        1 => "epic",
        2 => "outstanding",
        _ => "featured",
    };

    format!("https://raw.githubusercontent.com/GlobedGD/globed2/main/assets/globed-faces/{diffname}/{diffname}-{ratename}.png")
}

#[derive(Debug)]
pub enum WebhookError {
    Serialization(String),
    Request(reqwest::Error),
    RequestStatus((StatusCode, String)),
}

pub async fn send_webhook_messages(
    http_client: reqwest::Client,
    url: &str,
    messages: &[WebhookMessage],
    content: Option<String>,
) -> Result<(), WebhookError> {
    let mut embeds = Vec::new();

    for message in messages {
        if let Some(embed) = embed_for_message(message) {
            embeds.push(embed);
        }
    }

    let opts = WebhookOpts {
        username: None,
        content,
        embeds,
    };

    let response = http_client
        .post(url)
        .header("Content-Type", "application/json")
        .body(serde_json::to_string(&opts).map_err(|e| WebhookError::Serialization(e.to_string()))?)
        .send()
        .await
        .map_err(WebhookError::Request)?;

    let status = response.status();
    if !status.is_success() {
        let message = response.text().await.unwrap_or_else(|_| "<no response>".to_owned());

        return Err(WebhookError::RequestStatus((status, message)));
    }

    Ok(())
}
