ALTER TABLE users ADD COLUMN active_room_ban INTEGER REFERENCES punishments(punishment_id);
ALTER TABLE punishments ADD COLUMN type2 TEXT;
