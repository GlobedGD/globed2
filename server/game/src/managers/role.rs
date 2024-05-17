use crate::data::*;
use globed_shared::{warn, GameServerBootData, ServerRole, SyncMutex};
use rustc_hash::FxHashMap;

#[derive(Default)]
pub struct RoleManager {
    roles: SyncMutex<FxHashMap<String, ServerRole>>,
}

#[derive(Encodable, Decodable, DynamicSize, Default, Clone)]
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
    pub admin: bool,
}

impl ComputedRole {
    pub fn can_moderate(&self) -> bool {
        self.notices || self.notices_to_everyone || self.kick || self.kick_everyone || self.mute || self.ban || self.edit_role || self.admin
    }

    pub fn to_special_data(&self) -> Option<SpecialUserData> {
        if self.name_color.is_some() || !self.badge_icon.is_empty() || self.chat_color.is_some() {
            Some(SpecialUserData {
                badge_icon: if self.badge_icon.is_empty() {
                    None
                } else {
                    Some(self.badge_icon.clone().try_into().unwrap_or_default())
                },
                name_color: self.name_color.clone(),
                chat_color: self.chat_color.clone(),
            })
        } else {
            None
        }
    }
}

impl RoleManager {
    pub fn refresh_from(&self, gsbd: &GameServerBootData) {
        let mut roles = self.roles.lock();

        roles.clear();

        gsbd.roles.iter().for_each(|role| {
            // print warnings
            if role.badge_icon.len() > 32 {
                warn!("role id '{}' has a badge sprite of over 32 characters, this is not supported", role.id);
            }

            roles.insert(role.id.clone(), role.clone());
        });
    }

    pub fn get_all_roles(&self) -> Vec<ServerRole> {
        self.roles.lock().values().cloned().collect()
    }

    pub fn compute(&self, user_roles: &[String]) -> ComputedRole {
        let mut computed = ComputedRole::default();
        computed.priority = i32::MIN;

        let roles = self.roles.lock();
        for role_id in user_roles {
            let role = match roles.get(role_id) {
                Some(x) => x,
                None => {
                    warn!("trying to assign an invalid role to a user: {role_id}");
                    continue;
                }
            };

            let is_higher = role.priority > computed.priority;

            if is_higher {
                computed.priority = role.priority;

                if !role.badge_icon.is_empty() {
                    computed.badge_icon.copy_from_str(&role.badge_icon);
                }

                if !role.name_color.is_empty() {
                    computed.name_color = role.name_color.parse::<RichColor>().map(|x| Some(x)).unwrap_or_else(|x| {
                        warn!("failed to parse role name color for role {role_id}: {x}");
                        None
                    });

                    // if it's white, make it None
                    if computed
                        .name_color
                        .as_ref()
                        .is_some_and(|x| x.color.as_ref().first().is_some_and(|y| *y == Color3B { r: 255, g: 255, b: 255 }))
                    {
                        computed.name_color = None;
                    }
                }

                if !role.chat_color.is_empty() {
                    computed.chat_color = role.chat_color.parse::<Color3B>().map(|x| Some(x)).unwrap_or_else(|x| {
                        warn!("failed to parse role chat color for role {role_id}: {x}");
                        None
                    });

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
                    computed.admin = true;
                } else {
                    computed.notices |= role.notices;
                    computed.notices_to_everyone |= role.notices_to_everyone;
                    computed.kick |= role.kick;
                    computed.kick_everyone |= role.kick_everyone;
                    computed.mute |= role.mute;
                    computed.ban |= role.ban;
                    computed.edit_role |= role.edit_role;
                }
            }
        }

        computed
    }

    pub fn compute_priority(&self, user_roles: &[String]) -> i32 {
        let roles = self.roles.lock();

        user_roles
            .iter()
            .map(|r| roles.get(r).map(|x| x.priority).unwrap_or(i32::MIN))
            .max()
            .unwrap_or(i32::MIN)
    }

    // check if all provided role ids are valid and do exist
    pub fn all_valid(&self, user_roles: &[String]) -> bool {
        let roles = self.roles.lock();
        user_roles.iter().all(|x| roles.contains_key(x))
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
            admin: true,
            ..Default::default()
        }
    }
}
