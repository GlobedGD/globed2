-- Add up migration script here

CREATE TABLE featured_levels (
    id INTEGER PRIMARY KEY,
    level_id INTEGER NOT NULL,
    picked_at INTEGER NOT NULL,
    picked_by INTEGER NOT NULL,
    is_active SMALLINT NOT NULL DEFAULT 0
);
