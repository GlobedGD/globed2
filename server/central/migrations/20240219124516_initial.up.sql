-- Your SQL goes here

CREATE TABLE users (
    account_id INTEGER PRIMARY KEY,
    user_name TEXT,
    name_color TEXT,
    user_role SMALLINT NOT NULL DEFAULT 0,
    is_banned BOOLEAN NOT NULL DEFAULT FALSE,
    is_muted BOOLEAN NOT NULL DEFAULT FALSE,
    is_whitelisted BOOLEAN NOT NULL DEFAULT FALSE,
    admin_password TEXT,
    violation_reason TEXT,
    violation_expiry INTEGER
);