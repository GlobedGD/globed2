-- Add up migration script here
CREATE TABLE player_counts (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    log_time INTEGER NOT NULL,
    count INTEGER NOT NULL
);