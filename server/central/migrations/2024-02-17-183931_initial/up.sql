-- Your SQL goes here

CREATE TABLE users (
    account_id INTEGER PRIMARY KEY,
    name_color TEXT,
    user_role SMALLINT,
    is_banned SMALLINT,
    is_muted SMALLINT,
    is_whitelisted SMALLINT,
    admin_password TEXT,
);