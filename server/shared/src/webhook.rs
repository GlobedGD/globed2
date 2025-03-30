use std::borrow::Cow;

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
    AuthFail(String),                            // username
    NoticeToEveryone(String, usize, String),     // username, player count, message
    NoticeToSelection(String, usize, String),    // username, player count, message, reply id
    NoticeToPerson(String, String, String, u32), // author, target username, message, reply id
    NoticeReply(String, String, String, u32),    // author, mod who is being replied to, message, reply id
    KickEveryone(String, String),                // mod username, reason
    KickPerson(String, String, i32, String),     // mod username, target username, target account id, reason
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
pub struct WebhookAuthor<'a> {
    pub name: Cow<'a, str>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub icon_url: Option<Cow<'a, str>>,
}

#[derive(Serialize)]
pub struct WebhookFooter<'a> {
    pub text: Cow<'a, str>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub icon_url: Option<Cow<'a, str>>,
}

#[derive(Serialize)]
pub struct WebhookField<'a> {
    pub name: Cow<'a, str>,
    pub value: Cow<'a, str>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub inline: Option<bool>,
}

#[derive(Serialize)]
pub struct WebhookThumbnail<'a> {
    pub url: Cow<'a, str>,
}

#[derive(Default, Serialize)]
pub struct WebhookEmbed<'a> {
    pub title: Cow<'a, str>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub color: Option<u32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub author: Option<WebhookAuthor<'a>>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub description: Option<Cow<'a, str>>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub footer: Option<WebhookFooter<'a>>,
    #[serde(skip_serializing_if = "Vec::is_empty")]
    pub fields: Vec<WebhookField<'a>>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub thumbnail: Option<WebhookThumbnail<'a>>,
}

#[derive(Serialize)]
pub struct WebhookOpts<'a> {
    #[serde(skip_serializing_if = "Option::is_none")]
    pub username: Option<Cow<'a, str>>,

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
            title: Cow::Owned(format!("Global notice (for {player_count} people)")),
            color: hex_color_to_decimal("#4dace8"),
            description: Some(Cow::Owned(message.clone())),
            fields: vec![WebhookField {
                name: Cow::Borrowed("Performed by"),
                value: Cow::Owned(username.clone()),
                inline: Some(true),
            }],
            ..Default::default()
        }),
        WebhookMessage::NoticeToSelection(username, player_count, message) => Some(WebhookEmbed {
            title: Cow::Borrowed("Notice"),
            color: hex_color_to_decimal("#4dace8"),
            description: Some(Cow::Owned(message.clone())),
            fields: vec![
                WebhookField {
                    name: Cow::Borrowed("Performed by"),
                    value: Cow::Owned(username.clone()),
                    inline: Some(true),
                },
                WebhookField {
                    name: Cow::Borrowed("Sent to"),
                    value: Cow::Owned(format!("{player_count} people")),
                    inline: Some(true),
                },
            ],
            ..Default::default()
        }),
        WebhookMessage::NoticeToPerson(author, target, message, reply_id) => Some(WebhookEmbed {
            title: Cow::Owned(format!("Notice for {target}")),
            color: hex_color_to_decimal("#4dace8"),
            description: Some(Cow::Owned(message.clone())),
            fields: vec![
                WebhookField {
                    name: Cow::Borrowed("Performed by"),
                    value: Cow::Owned(author.clone()),
                    inline: Some(true),
                },
                WebhookField {
                    name: Cow::Borrowed("Can reply?"),
                    value: if *reply_id == 0 {
                        Cow::Borrowed("No")
                    } else {
                        Cow::Owned(format!("Yes, ID: {reply_id}"))
                    },
                    inline: Some(true),
                },
            ],
            ..Default::default()
        }),
        WebhookMessage::NoticeReply(author, mod_name, message, reply_id) => Some(WebhookEmbed {
            title: Cow::Owned(format!("Notice reply from {author}")),
            color: hex_color_to_decimal("#55d9ed"),
            description: Some(Cow::Owned(message.clone())),
            fields: vec![
                WebhookField {
                    name: Cow::Borrowed("Sent to"),
                    value: Cow::Owned(mod_name.clone()),
                    inline: Some(true),
                },
                WebhookField {
                    name: Cow::Borrowed("Reply ID"),
                    value: Cow::Owned(reply_id.to_string()),
                    inline: Some(true),
                },
            ],
            ..Default::default()
        }),
        WebhookMessage::KickEveryone(username, reason) => Some(WebhookEmbed {
            title: Cow::Borrowed("Kick everyone"),
            color: hex_color_to_decimal("#e8d34d"),
            description: Some(Cow::Owned(reason.clone())),
            fields: vec![WebhookField {
                name: Cow::Borrowed("Performed by"),
                value: Cow::Owned(username.clone()),
                inline: Some(true),
            }],
            ..Default::default()
        }),
        WebhookMessage::KickPerson(mod_name, user_name, target_id, reason) => Some(WebhookEmbed {
            title: Cow::Borrowed("User kicked"),
            color: hex_color_to_decimal("#e8d34d"),
            author: Some(WebhookAuthor {
                name: Cow::Owned(format!("{user_name} ({target_id})")),
                icon_url: None,
            }),
            description: Some(Cow::Owned(reason.clone())),
            fields: vec![WebhookField {
                name: Cow::Borrowed("Performed by"),
                value: Cow::Owned(mod_name.clone()),
                inline: Some(true),
            }],
            ..Default::default()
        }),
        WebhookMessage::UserBanned(bmsc) => Some(WebhookEmbed {
            title: Cow::Borrowed("User banned"),
            color: hex_color_to_decimal("#de3023"),
            author: Some(WebhookAuthor {
                name: Cow::Owned(format!("{} ({})", bmsc.target_name, bmsc.target_id)),
                icon_url: None,
            }),
            description: Some(Cow::Owned(bmsc.reason.clone().unwrap_or_else(|| "No reason given.".to_string()))),
            fields: vec![
                WebhookField {
                    name: Cow::Borrowed("Performed by"),
                    value: Cow::Owned(bmsc.mod_name.clone()),
                    inline: Some(true),
                },
                WebhookField {
                    name: Cow::Borrowed("Expires"),
                    value: if let Some(seconds) = bmsc.expiry {
                        Cow::Owned(format!("<t:{seconds}:f>"))
                    } else {
                        Cow::Borrowed("Permanent.")
                    },
                    inline: Some(true),
                },
            ],
            ..Default::default()
        }),

        WebhookMessage::UserUnbanned(removal) => Some(WebhookEmbed {
            title: Cow::Borrowed("User unbanned"),
            color: hex_color_to_decimal("31bd31"),
            author: Some(WebhookAuthor {
                name: Cow::Owned(format!("{} ({})", removal.name, removal.account_id)),
                icon_url: None,
            }),
            description: None,
            fields: vec![WebhookField {
                name: Cow::Borrowed("Performed by"),
                value: Cow::Owned(removal.mod_name.clone()),
                inline: Some(true),
            }],
            ..Default::default()
        }),
        WebhookMessage::UserMuted(bmsc) => Some(WebhookEmbed {
            title: Cow::Borrowed("User muted"),
            color: hex_color_to_decimal("#ded823"),
            author: Some(WebhookAuthor {
                name: Cow::Owned(format!("{} ({})", bmsc.target_name, bmsc.target_id)),
                icon_url: None,
            }),
            description: Some(Cow::Owned(bmsc.reason.clone().unwrap_or_else(|| "No reason given.".to_string()))),
            fields: vec![
                WebhookField {
                    name: Cow::Borrowed("Performed by"),
                    value: Cow::Owned(bmsc.mod_name.clone()),
                    inline: Some(true),
                },
                WebhookField {
                    name: Cow::Borrowed("Expires"),
                    value: if let Some(seconds) = bmsc.expiry {
                        Cow::Owned(format!("<t:{seconds}:f>"))
                    } else {
                        Cow::Borrowed("Permanent.")
                    },
                    inline: Some(true),
                },
            ],
            ..Default::default()
        }),

        WebhookMessage::UserUnmuted(removal) => Some(WebhookEmbed {
            title: Cow::Borrowed("User unmuted"),
            color: hex_color_to_decimal("#79bd31"),
            author: Some(WebhookAuthor {
                name: Cow::Owned(format!("{} ({})", removal.name, removal.account_id)),
                icon_url: None,
            }),
            description: None,
            fields: vec![WebhookField {
                name: Cow::Borrowed("Performed by"),
                value: Cow::Owned(removal.mod_name.clone()),
                inline: Some(true),
            }],
            ..Default::default()
        }),
        WebhookMessage::UserViolationMetaChanged(change) => Some(WebhookEmbed {
            title: Cow::Borrowed(if change.is_ban { "Ban state changed" } else { "Mute state changed" }),
            color: hex_color_to_decimal("#de7a23"),
            author: Some(WebhookAuthor {
                name: Cow::Owned(change.name.clone()),
                icon_url: None,
            }),
            fields: vec![
                WebhookField {
                    name: Cow::Borrowed("Performed by"),
                    value: Cow::Owned(change.mod_name.clone()),
                    inline: Some(false),
                },
                WebhookField {
                    name: Cow::Borrowed("Reason"),
                    value: Cow::Owned(change.reason.clone().unwrap_or_else(|| "No reason given.".to_owned())),
                    inline: Some(false),
                },
                WebhookField {
                    name: Cow::Borrowed("Expiration"),
                    value: Cow::Owned(change.expiry.map(|x| format!("<t:{x}:f>")).unwrap_or_else(|| "Permanent.".to_owned())),
                    inline: Some(false),
                },
            ],
            ..Default::default()
        }),
        WebhookMessage::UserRolesChanged(mod_name, user_name, new_roles) => Some(WebhookEmbed {
            title: Cow::Borrowed("Role change"),
            color: hex_color_to_decimal("#8b4de8"),
            author: Some(WebhookAuthor {
                name: Cow::Owned(user_name.clone()),
                icon_url: None,
            }),
            fields: vec![
                WebhookField {
                    name: Cow::Borrowed("Performed by"),
                    value: Cow::Owned(mod_name.clone()),
                    inline: Some(true),
                },
                WebhookField {
                    name: Cow::Borrowed("New roles"),
                    value: Cow::Owned(new_roles.join(", ")),
                    inline: Some(true),
                },
            ],
            ..Default::default()
        }),
        WebhookMessage::UserNameColorChanged(change) => Some(WebhookEmbed {
            title: Cow::Borrowed("Name color change"),
            color: hex_color_to_decimal(change.new_color.as_ref().map_or_else(|| "", |x| x.as_str())),
            author: Some(WebhookAuthor {
                name: Cow::Owned(change.name.clone()),
                icon_url: None,
            }),
            fields: vec![
                WebhookField {
                    name: Cow::Borrowed("Performed by"),
                    value: Cow::Owned(change.mod_name.clone()),
                    inline: Some(true),
                },
                WebhookField {
                    name: Cow::Borrowed("New color"),
                    value: Cow::Owned(change.new_color.clone().unwrap_or_else(|| "none".to_owned())),
                    inline: Some(true),
                },
            ],
            ..Default::default()
        }),
        WebhookMessage::FeaturedLevelSend(mod_id, mod_name, level_name, level_id, level_author, difficulty, rate_tier, notes) => Some(WebhookEmbed {
            title: Cow::Borrowed("New level send"),
            color: hex_color_to_decimal("#79bd31"),
            author: Some(WebhookAuthor {
                name: Cow::Owned(format!("{mod_name} ({mod_id})")),
                icon_url: None,
            }),
            fields: vec![
                WebhookField {
                    name: Cow::Borrowed("Level ID"),
                    value: Cow::Owned(level_id.to_string()),
                    inline: Some(true),
                },
                WebhookField {
                    name: Cow::Borrowed("Level Author"),
                    value: Cow::Owned(level_author.clone()),
                    inline: Some(true),
                },
                WebhookField {
                    name: Cow::Borrowed("Level Name"),
                    value: Cow::Owned(level_name.clone()),
                    inline: Some(true),
                },
                WebhookField {
                    name: Cow::Borrowed("Notes"),
                    value: Cow::Owned(notes.clone().unwrap_or_else(|| "None".to_owned())),
                    inline: Some(true),
                },
            ],
            thumbnail: Some(WebhookThumbnail {
                url: Cow::Owned(rate_tier_to_image(*difficulty, *rate_tier)),
            }),
            ..Default::default()
        }),
        WebhookMessage::LevelFeatured(level_name, level_id, level_author, difficulty, rate_tier) => Some(WebhookEmbed {
            title: Cow::Owned(format!("{level_name} by {level_author}")),
            color: hex_color_to_decimal("#7dfff5"),
            author: Some(WebhookAuthor {
                name: Cow::Borrowed("New Featured Level"),
                icon_url: None,
            }),
            fields: vec![WebhookField {
                name: Cow::Borrowed("Level ID"),
                value: Cow::Owned(level_id.to_string()),
                inline: Some(true),
            }],
            thumbnail: Some(WebhookThumbnail {
                url: Cow::Owned(rate_tier_to_image(*difficulty, *rate_tier)),
            }),
            ..Default::default()
        }),
        WebhookMessage::RoomCreated(room_id, room_name, username, account_id, hidden, protected) => Some(WebhookEmbed {
            title: Cow::Owned(room_name.clone()),
            color: hex_color_to_decimal("#4dace8"),
            author: Some(WebhookAuthor {
                name: Cow::Owned(format!("{username} ({account_id})")),
                icon_url: None,
            }),
            fields: vec![
                WebhookField {
                    name: Cow::Borrowed("Hidden"),
                    value: Cow::Borrowed(if *hidden { "Yes" } else { "No" }),
                    inline: Some(true),
                },
                WebhookField {
                    name: Cow::Borrowed("Private"),
                    value: Cow::Borrowed(if *protected { "Yes" } else { "No" }),
                    inline: Some(true),
                },
                WebhookField {
                    name: Cow::Borrowed("Room ID"),
                    value: Cow::Owned(room_id.to_string()),
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
