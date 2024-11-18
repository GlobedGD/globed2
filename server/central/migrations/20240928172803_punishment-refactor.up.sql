-- this is totally not made with help of ai ahh i hate sql

-- Step 1: Create the new punishments table
CREATE TABLE punishments (
    punishment_id INTEGER PRIMARY KEY AUTOINCREMENT,
    account_id INTEGER NOT NULL,
    type TEXT CHECK(type IN ('mute', 'ban')) NOT NULL, -- Punishment type, either mute or ban
    reason TEXT NOT NULL,
    expires_at INTEGER NOT NULL,
    issued_by INTEGER,
    FOREIGN KEY (issued_by) REFERENCES users (account_id),
    FOREIGN KEY (account_id) REFERENCES users (account_id)
);

-- Step 2: Add the new punishment reference columns to the users table
ALTER TABLE users ADD COLUMN active_mute INTEGER REFERENCES punishments(punishment_id);
ALTER TABLE users ADD COLUMN active_ban INTEGER REFERENCES punishments(punishment_id);

-- also remove this column lol
ALTER TABLE users DROP COLUMN user_role;

-- Step 3: Migrate existing ban data from users table to punishments table
INSERT INTO punishments (account_id, type, reason, expires_at)
SELECT account_id, 'ban', COALESCE(violation_reason, '') AS reason, COALESCE(violation_expiry, 0) AS expires_at
FROM users
WHERE is_banned = 1;

-- Step 4: Update users table to link to the new punishment records for bans
UPDATE users
SET active_ban = (
    SELECT punishment_id
    FROM punishments
    WHERE punishments.account_id = users.account_id
    AND type = 'ban'
);

-- Step 5: Migrate existing mute data from users table to punishments table
INSERT INTO punishments (account_id, type, reason, expires_at)
SELECT account_id, 'mute', COALESCE(violation_reason, '') AS reason, COALESCE(violation_expiry, 0) AS expires_at
FROM users
WHERE is_muted = 1;

-- Step 6: Update users table to link to the new punishment records for mutes
UPDATE users
SET active_mute = (
    SELECT punishment_id
    FROM punishments
    WHERE punishments.account_id = users.account_id
    AND type = 'mute'
);

-- Step 7: Drop the old ban/mute related columns from users table
ALTER TABLE users DROP COLUMN is_banned;
ALTER TABLE users DROP COLUMN is_muted;
ALTER TABLE users DROP COLUMN violation_reason;
ALTER TABLE users DROP COLUMN violation_expiry;
