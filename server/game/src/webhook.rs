use crate::server_thread::handlers::admin::*;
use serde::Serialize;

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
}

#[derive(Serialize)]
pub struct WebhookOpts<'a> {
    pub username: &'a str,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub content: Option<&'a str>,

    #[serde(skip_serializing_if = "Vec::is_empty")]
    pub embeds: Vec<WebhookEmbed<'a>>,
}

pub fn role_to_string(role: i32) -> String {
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
pub fn embed_for_message(message: &WebhookMessage) -> Option<WebhookEmbed> {
    match message {
        WebhookMessage::AuthFail(_user) => None,
        WebhookMessage::NoticeToEveryone(username, player_count, message) => Some(WebhookEmbed {
            title: format!("Global notice (for {player_count} people)"),
            color: hex_color_to_decimal("#4dace8"),
            author: Some(WebhookAuthor {
                name: username.clone(),
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
                name: username.clone(),
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
                name: target.clone(),
                icon_url: None,
            }),
            description: Some(message.clone()),
            footer: None,
            fields: vec![WebhookField {
                name: "Performed by",
                value: author.clone(),
                inline: Some(true),
            }],
        }),
        WebhookMessage::KickEveryone(username, reason) => Some(WebhookEmbed {
            title: "Kick everyone".to_owned(),
            color: hex_color_to_decimal("#e8d34d"),
            author: Some(WebhookAuthor {
                name: username.clone(),
                icon_url: None,
            }),
            description: Some(reason.clone()),
            footer: None,
            fields: Vec::new(),
        }),
        WebhookMessage::KickPerson(mod_name, user_name, target_id, reason) => Some(WebhookEmbed {
            title: "Kick user".to_owned(),
            color: hex_color_to_decimal("#e8d34d"),
            author: Some(WebhookAuthor {
                name: format!("{user_name} ({target_id})"),
                icon_url: None,
            }),
            description: Some(reason.clone()),
            footer: None,
            fields: vec![WebhookField {
                name: "Performed by",
                value: mod_name.clone(),
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
                name: format!("{} ({})", bmsc.target_name, bmsc.target_id),
                icon_url: None,
            }),
            description: if bmsc.new_state {
                Some(bmsc.reason.clone().unwrap_or_else(|| "No reason given.".to_string()))
            } else {
                None
            },
            footer: None,
            fields: if bmsc.new_state {
                vec![
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
                ]
            } else {
                vec![WebhookField {
                    name: "Performed by",
                    value: bmsc.mod_name.clone(),
                    inline: Some(true),
                }]
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
                name: format!("{} ({})", bmsc.target_name, bmsc.target_id),
                icon_url: None,
            }),
            description: if bmsc.new_state {
                Some(bmsc.reason.clone().unwrap_or_else(|| "No reason given.".to_string()))
            } else {
                None
            },
            footer: None,
            fields: if bmsc.new_state {
                vec![
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
                ]
            } else {
                vec![WebhookField {
                    name: "Performed by",
                    value: bmsc.mod_name.clone(),
                    inline: Some(true),
                }]
            },
        }),
        WebhookMessage::UserViolationMetaChanged(mod_name, user_name, expiry, reason) => Some(WebhookEmbed {
            title: "Ban/mute state changed".to_owned(),
            color: hex_color_to_decimal("#de7a23"),
            author: Some(WebhookAuthor {
                name: user_name.clone(),
                icon_url: None,
            }),
            description: None,
            footer: None,
            fields: vec![
                WebhookField {
                    name: "Performed by",
                    value: mod_name.clone(),
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
                name: user_name.clone(),
                icon_url: None,
            }),
            description: None,
            footer: None,
            fields: vec![
                WebhookField {
                    name: "Performed by",
                    value: mod_name.clone(),
                    inline: Some(true),
                },
                WebhookField {
                    name: "Old role",
                    value: role_to_string(*old_role),
                    inline: Some(true),
                },
                WebhookField {
                    name: "New role",
                    value: role_to_string(*new_role),
                    inline: Some(true),
                },
            ],
        }),
        WebhookMessage::UserNameColorChanged(mod_name, user_name, old_color, new_color) => Some(WebhookEmbed {
            title: "Name color change".to_owned(),
            color: hex_color_to_decimal(new_color.as_ref().map_or_else(|| "", |x| x.as_str())),
            author: Some(WebhookAuthor {
                name: user_name.clone(),
                icon_url: None,
            }),
            description: None,
            footer: None,
            fields: vec![
                WebhookField {
                    name: "Performed by",
                    value: mod_name.clone(),
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

pub fn hex_color_to_decimal(color: &str) -> Option<u32> {
    let color = color.strip_prefix('#').unwrap_or(color);

    u32::from_str_radix(color, 16).ok()
}
