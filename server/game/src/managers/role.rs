use crate::data::*;
use globed_shared::{GameServerBootData, IntMap, ServerRole, SyncMutex, error, warn};

#[derive(Default)]
pub struct RoleManager {
    roles: SyncMutex<IntMap<u8, ServerRole>>,
}

#[derive(Encodable, Decodable, DynamicSize, Default, Clone)]
pub struct GameServerRole {
    pub int_id: u8,
    pub role: ServerRole,
}

#[derive(Encodable, Decodable, DynamicSize, Default, Clone)]
#[allow(clippy::struct_excessive_bools)]
pub struct ComputedRole {
    pub priority: i32,
    pub badge_icon: FastString,
    pub name_color: Option<RichColor>,
    pub chat_color: Option<Color3B>,

    pub notices: bool,
    pub notices_to_everyone: bool,
    pub kick: bool,
    pub kick_everyone: bool,
    pub mute: bool,
    pub ban: bool,
    pub edit_role: bool,
    pub edit_featured_levels: bool,
    pub admin: bool,
}

impl ComputedRole {
    // check if the user has any mod perms at all
    pub fn can_moderate(&self) -> bool {
        self.notices
            || self.notices_to_everyone
            || self.kick
            || self.kick_everyone
            || self.mute
            || self.ban
            || self.edit_role
            || self.edit_featured_levels
            || self.admin
    }
}

impl RoleManager {
    pub fn refresh_from(&self, gsbd: &GameServerBootData) {
        let mut roles = self.roles.lock();

        roles.clear();

        if gsbd.roles.len() > u8::MAX.into() {
            error!("too many roles on the server ({})", gsbd.roles.len());
            error!(
                "for performance reasons, the role count is currently limited to {} and cannot be bypassed",
                u8::MAX
            );
            panic!("aborting due to too many roles");
        }

        gsbd.roles.iter().enumerate().for_each(|(idx, role)| {
            // print warnings
            if role.badge_icon.len() > 32 {
                warn!("role id '{}' has a badge sprite of over 32 characters, this is not supported", role.id);
            }

            // check if the color is valid
            if !role.name_color.is_empty() {
                match role.name_color.parse::<RichColor>() {
                    Ok(_) => {}
                    Err(e) => {
                        warn!("failed to parse the name color for role {}", role.id);
                        warn!("color string: '{}', error: {e}", role.name_color);
                    }
                }
            }

            roles.insert(idx as u8, role.clone());
        });
    }

    pub fn get_all_roles(&self) -> Vec<GameServerRole> {
        self.roles
            .lock()
            .iter()
            .map(|(int_id, x)| GameServerRole {
                int_id: *int_id,
                role: x.clone(),
            })
            .collect()
    }

    pub fn role_ids_to_int_ids(&self, ids: &[String]) -> FastVec<u8, 16> {
        let roles = self.roles.lock();
        ids.iter()
            .take(16)
            .filter_map(|id| roles.iter().find(|(_, role)| *role.id == *id).map(|(idx, _)| *idx))
            .collect()
    }

    pub fn compute(&self, user_roles: &[String]) -> ComputedRole {
        let mut computed = ComputedRole {
            priority: i32::MIN,
            ..Default::default()
        };

        let roles = self.roles.lock();
        for role_id in user_roles {
            let Some(role) = roles.values().find(|x| x.id == *role_id) else {
                warn!("trying to assign an invalid role to a user: {role_id}");
                continue;
            };

            let is_higher = role.priority > computed.priority;

            // if lower, update only if it's empty
            // if higher, always update
            if !role.badge_icon.is_empty() && (is_higher || computed.badge_icon.is_empty()) {
                computed.badge_icon.copy_from_str(&role.badge_icon);
            }

            if !role.name_color.is_empty() && (is_higher || computed.name_color.is_none()) {
                computed.name_color = role.name_color.parse::<RichColor>().map_or_else(
                    |x| {
                        warn!("failed to parse role name color for role {role_id}: {x}");
                        None
                    },
                    Some,
                );

                // if it's white, make it None
                if computed
                    .name_color
                    .as_ref()
                    .is_some_and(|x| x.color.as_ref().first().is_some_and(|y| *y == Color3B { r: 255, g: 255, b: 255 }))
                {
                    computed.name_color = None;
                }
            }

            if !role.chat_color.is_empty() && (is_higher || computed.chat_color.is_none()) {
                computed.chat_color = role.chat_color.parse::<Color3B>().map_or_else(
                    |x| {
                        warn!("failed to parse role chat color for role {role_id}: {x}");
                        None
                    },
                    Some,
                );

                // if it's white, make it None
                if computed.chat_color.as_ref().is_some_and(|x| *x == Color3B { r: 255, g: 255, b: 255 }) {
                    computed.chat_color = None;
                }
            }

            if role.admin {
                computed.notices = true;
                computed.notices_to_everyone = true;
                computed.kick = true;
                computed.kick_everyone = true;
                computed.mute = true;
                computed.ban = true;
                computed.edit_role = true;
                computed.edit_featured_levels = true;
                computed.admin = true;
            } else {
                computed.notices |= role.notices;
                computed.notices_to_everyone |= role.notices_to_everyone;
                computed.kick |= role.kick;
                computed.kick_everyone |= role.kick_everyone;
                computed.mute |= role.mute;
                computed.ban |= role.ban;
                computed.edit_role |= role.edit_role;
                computed.edit_featured_levels |= role.edit_featured_levels;
            }

            if is_higher {
                computed.priority = role.priority;
            }
        }

        computed
    }

    pub fn compute_priority(&self, user_roles: &[String]) -> i32 {
        let roles = self.roles.lock();

        user_roles
            .iter()
            .map(|r| roles.values().find(|role| *r == *role.id).map_or(i32::MIN, |x| x.priority))
            .max()
            .unwrap_or(i32::MIN)
    }

    // check if all provided role ids are valid and do exist
    pub fn all_valid(&self, user_roles: &[String]) -> bool {
        let roles = self.roles.lock();
        user_roles.iter().all(|x| roles.values().any(|y| *x == *y.id))
    }

    // get the default role of an non-admin-authenticated user
    #[inline]
    pub fn get_default(&self) -> ComputedRole {
        ComputedRole {
            priority: i32::MIN,
            ..Default::default()
        }
    }

    // get the role with highest perms
    #[inline]
    pub fn get_superadmin(&self) -> ComputedRole {
        ComputedRole {
            priority: i32::MAX,
            notices: true,
            notices_to_everyone: true,
            kick: true,
            kick_everyone: true,
            mute: true,
            ban: true,
            edit_role: true,
            edit_featured_levels: true,
            admin: true,
            ..Default::default()
        }
    }
}
