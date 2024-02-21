use std::time::{SystemTime, UNIX_EPOCH};

use globed_shared::UserEntry;
use rocket_db_pools::sqlx::{query_as, Result};
use sqlx::{prelude::*, query, sqlite::SqliteRow};

use super::GlobedDb;

struct UserEntryWrapper(UserEntry);

impl<'r> FromRow<'r, SqliteRow> for UserEntryWrapper {
    fn from_row(row: &'r SqliteRow) -> Result<Self, sqlx::Error> {
        let account_id = row.try_get("account_id")?;
        let user_name: Option<String> = row.try_get("user_name")?;
        let name_color: Option<String> = row.try_get("name_color")?;
        let user_role = row.try_get("user_role")?;
        let is_banned = row.try_get("is_banned")?;
        let is_muted = row.try_get("is_muted")?;
        let is_whitelisted = row.try_get("is_whitelisted")?;
        let admin_password: Option<String> = row.try_get("admin_password")?;
        let violation_reason: Option<String> = row.try_get("violation_reason")?;
        let violation_expiry = row.try_get("violation_expiry")?;

        Ok(UserEntryWrapper(UserEntry {
            account_id,
            user_name: user_name.and_then(|x| x.try_into().ok()),
            name_color: name_color.and_then(|x| x.try_into().ok()),
            user_role,
            is_banned,
            is_muted,
            is_whitelisted,
            admin_password: admin_password.and_then(|x| x.try_into().ok()),
            violation_reason: violation_reason.and_then(|x| x.try_into().ok()),
            violation_expiry,
        }))
    }
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
        let res: Option<UserEntryWrapper> = query_as("SELECT * FROM users WHERE user_name = ?")
            .bind(name)
            .fetch_optional(&self.0)
            .await?;

        self.unwrap_user(res).await
    }

    pub async fn update_user(&self, account_id: i32, user: &UserEntry) -> Result<()> {
        // convert Option<FastString> into Option<String>
        let name_color = user.name_color.clone().map(|x| x.try_to_string());
        let user_name = user.user_name.clone().map(|x| x.try_to_string());
        let admin_password = user.admin_password.clone().map(|x| x.try_to_string());
        let violation_reason = user.violation_reason.clone().map(|x| x.try_to_string());

        query("UPDATE users SET user_name = ?, name_color = ?, user_role = ?, is_banned = ?, is_muted = ?, is_whitelisted = ?, admin_password = ?, violation_reason = ?, violation_expiry = ? WHERE account_id = ?")
            .bind(user_name)
            .bind(name_color)
            .bind(user.user_role)
            .bind(user.is_banned)
            .bind(user.is_muted)
            .bind(user.is_whitelisted)
            .bind(admin_password)
            .bind(violation_reason)
            .bind(user.violation_expiry)
            .bind(account_id)
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
}
