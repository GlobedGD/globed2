-- Add up migration script here
ALTER TABLE users ADD COLUMN admin_password_hash TEXT;