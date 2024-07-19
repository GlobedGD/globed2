-- Add down migration script here
ALTER TABLE users DROP COLUMN admin_password_hash;