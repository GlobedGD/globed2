use std::time::{Duration, SystemTime, UNIX_EPOCH};

use globed_shared::UserEntry;
use rocket_db_pools::sqlx::{query_as, Result};
use serde::Serialize;
use sqlx::{prelude::*, query, sqlite::SqliteRow};

use super::GlobedDb;

struct UserEntryWrapper(UserEntry);

impl<'r> FromRow<'r, SqliteRow> for UserEntryWrapper {
    fn from_row(row: &'r SqliteRow) -> Result<Self, sqlx::Error> {
        let account_id = row.try_get("account_id")?;
        let user_name = row.try_get("user_name")?;
        let name_color = row.try_get("name_color")?;
        let user_role = row.try_get("user_role")?;
        let is_banned = row.try_get("is_banned")?;
        let is_muted = row.try_get("is_muted")?;
        let is_whitelisted = row.try_get("is_whitelisted")?;
        let admin_password = row.try_get("admin_password")?;
        let violation_reason = row.try_get("violation_reason")?;
        let violation_expiry = row.try_get("violation_expiry")?;

        Ok(UserEntryWrapper(UserEntry {
            account_id,
            user_name,
            name_color,
            user_role,
            is_banned,
            is_muted,
            is_whitelisted,
            admin_password,
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

impl GlobedDb {
    pub async fn get_user(&self, account_id: i32) -> Result<Option<UserEntry>> {
        let res: Option<UserEntryWrapper> = query_as("SELECT * FROM users WHERE account_id = ?")
            .bind(account_id)
            .fetch_optional(&self.0)
            .await?;

        self.unwrap_user(res).await
    }

    pub async fn get_user_by_name(&self, name: &str) -> Result<Option<UserEntry>> {
        // we do this weird clause so that an exact match would be selected first
        let res: Option<UserEntryWrapper> = query_as("SELECT * FROM users WHERE user_name LIKE ? OR user_name LIKE ?")
            .bind(name)
            .bind(format!("%{name}%"))
            .fetch_optional(&self.0)
            .await?;

        self.unwrap_user(res).await
    }

    pub async fn update_user(&self, account_id: i32, user: &UserEntry) -> Result<()> {
        query(
            "INSERT OR REPLACE INTO users (account_id, user_name, name_color, user_role, is_banned, is_muted, is_whitelisted, admin_password, violation_reason, violation_expiry)
                    VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)")
            .bind(account_id)
            .bind(&user.user_name)
            .bind(&user.name_color)
            .bind(user.user_role)
            .bind(user.is_banned)
            .bind(user.is_muted)
            .bind(user.is_whitelisted)
            .bind(&user.admin_password)
            .bind(&user.violation_reason)
            .bind(user.violation_expiry)
            .execute(&self.0)
            .await
            .map(|_| ())
    }

    #[allow(clippy::cast_possible_wrap)]
    async fn maybe_expire_ban(&self, user: &mut UserEntry) -> Result<()> {
        let expired = user.violation_expiry.as_ref().is_some_and(|expiry| {
            let current_time = SystemTime::now()
                .duration_since(UNIX_EPOCH)
                .expect("clock went backwards")
                .as_secs() as i64;

            current_time > *expiry
        });

        if expired {
            user.is_banned = false;
            user.is_muted = false;
            user.violation_reason = None;
            user.violation_expiry = None;

            self.update_user(user.account_id, user).await?;
        }

        Ok(())
    }

    /// Convert a `Option<UserEntryWrapper>` into `Option<UserEntry>` and expire their ban/mute if needed.
    #[inline]
    async fn unwrap_user(&self, user: Option<UserEntryWrapper>) -> Result<Option<UserEntry>> {
        let mut user = user.map(|x| x.0);

        // if they are banned/muted and the ban/mute expired, unban/unmute them
        if user.as_ref().is_some_and(|user| user.is_banned || user.is_muted) {
            let user = user.as_mut().unwrap();
            self.maybe_expire_ban(user).await?;
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

        let delete_before =
            i64::try_from((SystemTime::now().duration_since(UNIX_EPOCH).unwrap() - Duration::from_days(31)).as_secs())
                .unwrap_or(0);

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
}
