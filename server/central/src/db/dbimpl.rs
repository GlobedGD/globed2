use std::time::{Duration, SystemTime, UNIX_EPOCH};

use globed_shared::{AdminPunishUserAction, PunishmentType, ServerUserEntry, UserPunishment, debug};
use rocket_db_pools::sqlx::{Result, query_as};
use serde::Serialize;
use sqlx::{prelude::*, query, query_scalar, sqlite::SqliteRow};

use super::GlobedDb;

struct UserEntryWrapper(pub ServerUserEntry);
struct UserPunishmentWrapper(pub UserPunishment);

impl<'r> FromRow<'r, SqliteRow> for UserEntryWrapper {
    fn from_row(row: &'r SqliteRow) -> Result<Self, sqlx::Error> {
        let account_id = row.try_get("account_id")?;
        let user_name = row.try_get("user_name")?;
        let name_color = row.try_get("name_color")?;

        let user_roles: Option<String> = row.try_get("user_roles")?;
        let mut user_roles = user_roles.map_or(Vec::new(), |s| s.split(',').map(|x| x.to_owned()).collect::<Vec<_>>());
        user_roles.retain(|x| !x.is_empty());

        let is_whitelisted = row.try_get("is_whitelisted")?;
        let admin_password = row.try_get("admin_password")?;
        let admin_password_hash = row.try_get("admin_password_hash")?;
        let active_mute = row.try_get("active_mute")?;
        let active_ban = row.try_get("active_ban")?;

        Ok(UserEntryWrapper(ServerUserEntry {
            account_id,
            user_name,
            name_color,
            user_roles,
            is_whitelisted,
            admin_password,
            admin_password_hash,
            active_mute,
            active_ban,
        }))
    }
}

impl<'r> FromRow<'r, SqliteRow> for UserPunishmentWrapper {
    fn from_row(row: &'r SqliteRow) -> Result<Self, sqlx::Error> {
        let id = row.try_get("punishment_id")?;
        let account_id = row.try_get("account_id")?;
        let r#typestr: String = row.try_get("type")?;
        let reason = row.try_get("reason")?;

        let expires_at = row.try_get("expires_at")?;
        let issued_at = row.try_get("issued_at")?;
        let issued_by = row.try_get("issued_by")?;

        let r#type = if r#typestr == "ban" { PunishmentType::Ban } else { PunishmentType::Mute };

        Ok(UserPunishmentWrapper(UserPunishment {
            id,
            account_id,
            r#type,
            reason,
            expires_at,
            issued_at,
            issued_by,
        }))
    }
}

#[derive(Clone, FromRow, Serialize)]
pub struct PlayerCountHistoryEntry {
    #[serde(skip_serializing)]
    #[allow(dead_code)]
    id: i64,
    log_time: i64,
    count: i64,
}

#[derive(Clone, FromRow, Serialize)]
pub struct FullFeaturedLevel {
    id: i64,
    level_id: i32,
    picked_at: i32,
    picked_by: i32,
    is_active: bool,
    rate_tier: i32,
}

#[derive(Clone, FromRow, Serialize)]
pub struct FeaturedLevel {
    id: i64,
    level_id: i32,
    rate_tier: i32,
}

#[derive(Clone, Serialize)]
pub struct FeaturedLevelPage {
    pub levels: Vec<FeaturedLevel>,
    pub page: usize,
    pub is_last_page: bool,
}

impl GlobedDb {
    pub async fn get_user(&self, account_id: i32) -> Result<Option<ServerUserEntry>> {
        let res: Option<UserEntryWrapper> = query_as("SELECT * FROM users WHERE account_id = ?")
            .bind(account_id)
            .fetch_optional(&self.0)
            .await?;

        self.unwrap_user(res).await
    }

    pub async fn get_user_by_name(&self, name: &str) -> Result<Option<ServerUserEntry>> {
        // we do this weird clause so that an exact match would be selected first
        let res: Option<UserEntryWrapper> = query_as("SELECT * FROM users WHERE user_name LIKE ? OR user_name LIKE ?")
            .bind(name)
            .bind(format!("%{name}%"))
            .fetch_optional(&self.0)
            .await?;

        self.unwrap_user(res).await
    }

    pub async fn insert_empty_user(&self, account_id: i32) -> Result<()> {
        query!("INSERT INTO users (account_id) VALUES (?) ON CONFLICT DO NOTHING", account_id)
            .execute(&self.0)
            .await
            .map(|_| ())
    }

    pub async fn get_punishment(&self, id: i64) -> Result<Option<UserPunishment>> {
        Ok(query_as::<_, UserPunishmentWrapper>("SELECT * FROM punishments WHERE punishment_id = ?")
            .bind(id)
            .fetch_optional(&self.0)
            .await?
            .map(|x| x.0))
    }

    pub async fn get_active_ban(&self, account_id: i32) -> Result<UserPunishment> {
        query_as::<_, UserPunishmentWrapper>(
            r#"
                SELECT p.punishment_id, p.type, p.account_id, p.reason, p.expires_at, p.issued_at, p.issued_by
                FROM users u
                LEFT JOIN punishments p ON u.active_ban = punishment_id
                WHERE u.account_id = ?
            "#,
        )
        .bind(account_id)
        .fetch_one(&self.0)
        .await
        .map(|x| x.0)
    }

    pub async fn get_active_mute(&self, account_id: i32) -> Result<UserPunishment> {
        query_as::<_, UserPunishmentWrapper>(
            r#"
                SELECT p.punishment_id, p.type, p.account_id, p.reason, p.expires_at, p.issued_at, p.issued_by
                FROM users u
                LEFT JOIN punishments p ON u.active_mute = punishment_id
                WHERE u.account_id = ?
            "#,
        )
        .bind(account_id)
        .fetch_one(&self.0)
        .await
        .map(|x| x.0)
    }

    /// get users' ban and mute punishments respectively
    pub async fn get_users_punishments(&self, user: &ServerUserEntry) -> Result<[Option<UserPunishment>; 2]> {
        let ban = match user.active_ban {
            Some(id) => match self.get_punishment(id).await {
                Ok(x) => x,
                Err(e) => return Err(e),
            },
            None => None,
        };

        let mute = match user.active_mute {
            Some(id) => match self.get_punishment(id).await {
                Ok(x) => x,
                Err(e) => return Err(e),
            },
            None => None,
        };

        Ok([ban, mute])
    }

    pub async fn get_all_user_punishments(&self, account_id: i32) -> Result<Vec<UserPunishment>> {
        query_as::<_, UserPunishmentWrapper>("SELECT * FROM punishments WHERE account_id = ? ORDER BY punishment_id DESC LIMIT 100")
            .bind(account_id)
            .fetch_all(&self.0)
            .await
            .map(|vec| vec.into_iter().map(|x| x.0).collect())
    }

    async fn maybe_expire_punishments(&self, user: &mut ServerUserEntry) -> Result<()> {
        let punishments = self.get_users_punishments(user).await?;

        if punishments[0].as_ref().is_some_and(|ban| ban.expired()) {
            user.active_ban = None;
        }

        if punishments[1].as_ref().is_some_and(|mute| mute.expired()) {
            user.active_mute = None;
        }

        // additional sanity checks, just in case.

        if punishments[0].is_none() && user.active_ban.is_some() {
            user.active_ban = None;
        }

        if punishments[1].is_none() && user.active_mute.is_some() {
            user.active_mute = None;
        }

        Ok(())
    }

    async fn migrate_admin_password(&self, user: &mut ServerUserEntry) -> Result<()> {
        if let Some(password) = user.admin_password.as_ref() {
            let hash = globed_shared::generate_argon2_hash(password);
            debug!("generating hash for password {password}: {hash}");

            query("UPDATE users SET admin_password = NULL, admin_password_hash = ? WHERE account_id = ?")
                .bind(&hash)
                .bind(user.account_id)
                .execute(&self.0)
                .await?;

            user.admin_password_hash = Some(hash);
        };

        user.admin_password = None;

        Ok(())
    }

    /// Convert a `Option<UserEntryWrapper>` into `Option<ServerUserEntry>` and expire their ban/mute if needed.
    #[inline]
    async fn unwrap_user(&self, user: Option<UserEntryWrapper>) -> Result<Option<ServerUserEntry>> {
        let mut user = user.map(|x| x.0);

        // if they are banned/muted and the ban/mute expired, unban/unmute them
        if user.as_ref().is_some_and(|user| user.active_mute.is_some() || user.active_ban.is_some()) {
            let user = user.as_mut().unwrap();
            self.maybe_expire_punishments(user).await?;
        }

        // TODO: remove this in the near future
        // migrate admin password to hashed form
        if user.as_ref().is_some_and(|user| user.admin_password.is_some()) {
            let user = user.as_mut().unwrap();
            self.migrate_admin_password(user).await?;
        }

        Ok(user)
    }

    // User update actions

    pub async fn update_username(&self, account_id: i32, new_name: &str) -> Result<()> {
        query!("UPDATE users SET user_name = ? WHERE account_id = ?", new_name, account_id)
            .execute(&self.0)
            .await
            .map(|_| ())
    }

    pub async fn update_user_name_color(&self, account_id: i32, color: &str) -> Result<()> {
        // make sure the user exists in the db
        self.insert_empty_user(account_id).await?;

        query!("UPDATE users SET name_color = ? WHERE account_id = ?", color, account_id)
            .execute(&self.0)
            .await
            .map(|_| ())
    }

    pub async fn update_user_roles(&self, account_id: i32, roles: &[String]) -> Result<()> {
        let joined = roles.join(",");

        // make sure the user exists in the db
        self.insert_empty_user(account_id).await?;

        query!("UPDATE users SET user_roles = ? WHERE account_id = ?", joined, account_id)
            .execute(&self.0)
            .await
            .map(|_| ())
    }

    pub async fn punish_user(&self, action: &AdminPunishUserAction) -> Result<i64> {
        let reason = action.reason.try_to_str();
        let r#type = if action.is_ban { "ban" } else { "mute" };
        let issued_at = UNIX_EPOCH.elapsed().unwrap().as_secs() as i64;

        let expires_at = action.expires_at as i64;

        // make sure both the mod and the user exist in the db
        self.insert_empty_user(action.account_id).await?;
        self.insert_empty_user(action.issued_by).await?;

        let id = query!(
            "INSERT INTO punishments (account_id, type, reason, expires_at, issued_at, issued_by) VALUES (?, ?, ?, ?, ?, ?) RETURNING punishment_id",
            action.account_id,
            r#type,
            reason,
            expires_at,
            issued_at,
            action.issued_by
        )
        .fetch_one(&self.0)
        .await?
        .punishment_id;

        // set the punishment
        if action.is_ban {
            query!("UPDATE users SET active_ban = ? WHERE account_id = ?", id, action.account_id)
                .execute(&self.0)
                .await?;
        } else {
            query!("UPDATE users SET active_mute = ? WHERE account_id = ?", id, action.account_id)
                .execute(&self.0)
                .await?;
        }

        Ok(id)
    }

    pub async fn unpunish_user(&self, account_id: i32, is_ban: bool) -> Result<()> {
        // make sure the user exists in the db
        self.insert_empty_user(account_id).await?;

        if is_ban {
            query!("UPDATE users SET active_ban = NULL WHERE account_id = ?", account_id)
                .execute(&self.0)
                .await?;
        } else {
            query!("UPDATE users SET active_mute = NULL WHERE account_id = ?", account_id)
                .execute(&self.0)
                .await?;
        }

        Ok(())
    }

    pub async fn whitelist_user(&self, account_id: i32, state: bool) -> Result<()> {
        // make sure the user exists in the db
        self.insert_empty_user(account_id).await?;

        query!("UPDATE users SET is_whitelisted = ? WHERE account_id = ?", state, account_id)
            .execute(&self.0)
            .await?;

        Ok(())
    }

    pub async fn update_user_admin_password(&self, account_id: i32, password: &str) -> Result<()> {
        // make sure the user exists in the db
        self.insert_empty_user(account_id).await?;

        let hash = globed_shared::generate_argon2_hash(password);

        query!("UPDATE users SET admin_password_hash = ? WHERE account_id = ?", hash, account_id)
            .execute(&self.0)
            .await
            .map(|_| ())
    }

    pub async fn edit_punishment(&self, account_id: i32, is_ban: bool, reason: &str, expires_at: u64) -> Result<()> {
        let punishment = if is_ban {
            self.get_active_ban(account_id).await?
        } else {
            self.get_active_mute(account_id).await?
        };

        let expires_at = expires_at as i64;

        query!(
            "UPDATE punishments SET reason = ?, expires_at = ? WHERE punishment_id = ?",
            reason,
            expires_at,
            punishment.id
        )
        .execute(&self.0)
        .await
        .map(|_| ())
    }

    // Misc

    pub async fn insert_player_count_history(&self, entries: &[(SystemTime, u32)]) -> Result<()> {
        for entry in entries {
            query("INSERT INTO player_counts (log_time, count) VALUES (?, ?)")
                .bind(i64::try_from(entry.0.duration_since(UNIX_EPOCH).unwrap().as_secs()).unwrap_or(0))
                .bind(entry.1)
                .execute(&self.0)
                .await?;
        }

        // delete logs older than 1 month

        let delete_before = i64::try_from((UNIX_EPOCH.elapsed().unwrap() - Duration::from_days(31)).as_secs()).unwrap_or(0);

        if delete_before != 0 {
            query("DELETE FROM player_counts WHERE log_time < ?")
                .bind(delete_before)
                .execute(&self.0)
                .await?;
        }

        Ok(())
    }

    pub async fn fetch_player_count_history(&self, after: SystemTime) -> Result<Vec<PlayerCountHistoryEntry>> {
        query_as::<_, PlayerCountHistoryEntry>("SELECT * FROM player_counts WHERE log_time > ?")
            .bind(i64::try_from(after.duration_since(UNIX_EPOCH).unwrap().as_secs()).unwrap_or(0))
            .fetch_all(&self.0)
            .await
    }

    pub async fn get_current_featured_level(&self) -> Result<Option<FeaturedLevel>> {
        let mut result = query_as::<_, FeaturedLevel>("SELECT id, level_id, rate_tier FROM featured_levels WHERE is_active = 1 LIMIT 1")
            .fetch_all(&self.0)
            .await?;

        Ok(result.pop())
    }

    pub async fn get_featured_level_history_new(&self, page: usize) -> Result<FeaturedLevelPage> {
        const PAGE_SIZE: usize = 10;

        let levels = query_as::<_, FeaturedLevel>("SELECT id, level_id, rate_tier FROM featured_levels ORDER BY id DESC LIMIT ? OFFSET ?")
            .bind(PAGE_SIZE as i64)
            .bind((page * PAGE_SIZE) as i64)
            .fetch_all(&self.0)
            .await?;

        Ok(FeaturedLevelPage {
            page,
            is_last_page: levels.len() < 10,
            levels,
        })
    }

    pub async fn get_featured_level_history(&self, page: usize) -> Result<Vec<FeaturedLevel>> {
        const PAGE_SIZE: usize = 10;

        let levels = query_as::<_, FeaturedLevel>("SELECT id, level_id, rate_tier FROM featured_levels ORDER BY id DESC LIMIT ? OFFSET ?")
            .bind(PAGE_SIZE as i64)
            .bind((page * PAGE_SIZE) as i64)
            .fetch_all(&self.0)
            .await?;

        Ok(levels)
    }

    pub async fn replace_featured_level(&self, account_id: i32, level_id: i32, rate_tier: i32) -> Result<()> {
        // set all previous levels to inactive
        query("UPDATE featured_levels SET is_active = 0").execute(&self.0).await?;

        query("INSERT OR REPLACE INTO featured_levels (level_id, picked_at, picked_by, is_active, rate_tier) VALUES (?, ?, ?, ?, ?)")
            .bind(level_id)
            .bind(i64::try_from(UNIX_EPOCH.elapsed().unwrap().as_secs()).unwrap_or(0))
            .bind(account_id)
            .bind(1)
            .bind(rate_tier)
            .execute(&self.0)
            .await?;

        Ok(())
    }

    pub async fn has_been_featured(&self, level_id: i32) -> Result<bool> {
        let count: i64 = query_scalar("SELECT COUNT(*) from featured_levels WHERE level_id = ?")
            .bind(level_id)
            .fetch_one(&self.0)
            .await?;

        Ok(count > 0)
    }
}
