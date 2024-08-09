use std::time::{Duration, SystemTime, UNIX_EPOCH};

use globed_shared::{debug, ServerUserEntry, UserEntry};
use rocket_db_pools::sqlx::{query_as, Result};
use serde::Serialize;
use sqlx::{prelude::*, query, query_scalar, sqlite::SqliteRow};

use super::GlobedDb;

struct UserEntryWrapper(ServerUserEntry);

impl<'r> FromRow<'r, SqliteRow> for UserEntryWrapper {
    fn from_row(row: &'r SqliteRow) -> Result<Self, sqlx::Error> {
        let account_id = row.try_get("account_id")?;
        let user_name = row.try_get("user_name")?;
        let name_color = row.try_get("name_color")?;

        let user_roles: Option<String> = row.try_get("user_roles")?;
        let mut user_roles = user_roles.map_or(Vec::new(), |s| s.split(',').map(|x| x.to_owned()).collect::<Vec<_>>());
        user_roles.retain(|x| !x.is_empty());

        let is_banned: bool = row.try_get("is_banned")?;
        let is_muted = row.try_get("is_muted")?;
        let is_whitelisted = row.try_get("is_whitelisted")?;
        let admin_password = row.try_get("admin_password")?;
        let admin_password_hash = row.try_get("admin_password_hash")?;
        let violation_reason = row.try_get("violation_reason")?;
        let violation_expiry = row.try_get("violation_expiry")?;

        Ok(UserEntryWrapper(ServerUserEntry {
            account_id,
            user_name,
            name_color,
            user_roles,
            is_banned,
            is_muted,
            is_whitelisted,
            admin_password,
            admin_password_hash,
            violation_reason,
            violation_expiry,
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

    pub async fn update_user_server(&self, account_id: i32, user: &ServerUserEntry) -> Result<()> {
        // oh god i hate myself
        query(
            r"
            INSERT INTO users (account_id, user_name, name_color, user_roles, is_banned, is_muted, is_whitelisted, violation_reason, violation_expiry)
                    VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
            ON CONFLICT(account_id) DO UPDATE SET
                user_name = excluded.user_name,
                name_color = excluded.name_color,
                user_roles = excluded.user_roles,
                is_banned = excluded.is_banned,
                is_muted = excluded.is_muted,
                is_whitelisted = excluded.is_whitelisted,
                violation_reason = excluded.violation_reason,
                violation_expiry = excluded.violation_expiry;
            ",
        )
        .bind(account_id)
        .bind(&user.user_name)
        .bind(&user.name_color)
        .bind(user.user_roles.join(","))
        .bind(user.is_banned)
        .bind(user.is_muted)
        .bind(user.is_whitelisted)
        .bind(&user.violation_reason)
        .bind(user.violation_expiry)
        .execute(&self.0)
        .await?;

        // update admin password
        if let Some(password) = user.admin_password.as_ref() {
            let hash = globed_shared::generate_argon2_hash(password);
            debug!("generating hash for password {password}: {hash}");
            query("UPDATE users SET admin_password = NULL, admin_password_hash = ? WHERE account_id = ?")
                .bind(hash)
                .bind(account_id)
                .execute(&self.0)
                .await?;
        };

        Ok(())
    }

    pub async fn update_user(&self, account_id: i32, user: UserEntry) -> Result<()> {
        self.update_user_server(account_id, &ServerUserEntry::from_user_entry(user)).await
    }

    async fn maybe_expire_ban(&self, user: &mut ServerUserEntry) -> Result<()> {
        let expired = user.violation_expiry.as_ref().is_some_and(|expiry| {
            let current_time = SystemTime::now().duration_since(UNIX_EPOCH).expect("clock went backwards").as_secs() as i64;

            current_time > *expiry
        });

        if expired {
            user.is_banned = false;
            user.is_muted = false;
            user.violation_reason = None;
            user.violation_expiry = None;

            self.update_user_server(user.account_id, user).await?;
        }

        Ok(())
    }

    async fn migrate_admin_password(&self, user: &mut ServerUserEntry) -> Result<()> {
        // update_user already does the migration itself
        self.update_user_server(user.account_id, user).await
    }

    /// Convert a `Option<UserEntryWrapper>` into `Option<UserEntry>` and expire their ban/mute if needed.
    #[inline]
    async fn unwrap_user(&self, user: Option<UserEntryWrapper>) -> Result<Option<ServerUserEntry>> {
        let mut user = user.map(|x| x.0);

        // if they are banned/muted and the ban/mute expired, unban/unmute them
        if user.as_ref().is_some_and(|user| user.is_banned || user.is_muted) {
            let user = user.as_mut().unwrap();
            self.maybe_expire_ban(user).await?;
        }

        // migrate admin password to hashed form
        if user.as_ref().is_some_and(|user| user.admin_password.is_some()) {
            let user = user.as_mut().unwrap();
            self.migrate_admin_password(user).await?;
        }

        Ok(user)
    }

    pub async fn insert_player_count_history(&self, entries: &[(SystemTime, u32)]) -> Result<()> {
        for entry in entries {
            query("INSERT INTO player_counts (log_time, count) VALUES (?, ?)")
                .bind(i64::try_from(entry.0.duration_since(UNIX_EPOCH).unwrap().as_secs()).unwrap_or(0))
                .bind(entry.1)
                .execute(&self.0)
                .await?;
        }

        // delete logs older than 1 month

        let delete_before = i64::try_from((SystemTime::now().duration_since(UNIX_EPOCH).unwrap() - Duration::from_days(31)).as_secs()).unwrap_or(0);

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
            .bind(i64::try_from(SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_secs()).unwrap_or(0))
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
