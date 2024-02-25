use super::*;
use crate::{data::*, server_thread::GameServerThread};

impl GameServerThread {
    gs_handler!(self, handle_sync_icons, SyncIconsPacket, packet, {
        let _ = gs_needauth!(self);

        self.account_data.lock().icons.clone_from(&packet.icons);
        Ok(())
    });

    gs_handler!(self, handle_request_global_list, RequestGlobalPlayerListPacket, _packet, {
        let _ = gs_needauth!(self);

        let player_count = self.game_server.state.player_count.load(Ordering::Relaxed) as usize;
        let encoded_size = size_of_types!(u32) + size_of_types!(PlayerPreviewAccountData) * player_count;

        self.send_packet_alloca_with::<GlobalPlayerListPacket, _>(encoded_size, |buf| {
            buf.write_list_with(player_count, |buf| {
                self.game_server.for_every_player_preview(
                    move |preview, count, buf| {
                        // we do additional length check because player count may have increased since then
                        if count < player_count {
                            buf.write_value(preview);
                            true
                        } else {
                            false
                        }
                    },
                    buf,
                )
            });
        })
        .await
    });

    gs_handler!(self, handle_create_room, CreateRoomPacket, _packet, {
        let account_id = gs_needauth!(self);

        // if we are already in a room, just return the same room ID, otherwise create a new one
        let mut room_id: u32 = self.room_id.load(Ordering::Relaxed);

        if room_id == 0 {
            room_id = self.game_server.state.room_manager.create_room(account_id);
            self.room_id.store(room_id, Ordering::Relaxed);
        }

        self.send_packet_static(&RoomCreatedPacket { room_id }).await
    });

    gs_handler!(self, handle_join_room, JoinRoomPacket, packet, {
        let account_id = gs_needauth!(self);

        if !self.game_server.state.room_manager.is_valid_room(packet.room_id) {
            return self.send_packet_static(&RoomJoinFailedPacket).await;
        }

        let old_room_id = self.room_id.swap(packet.room_id, Ordering::Relaxed);
        let level_id = self.level_id.load(Ordering::Relaxed);

        // even if we were in the global room beforehand, remove the player from there
        self.game_server.state.room_manager.with_any(old_room_id, |pm| {
            pm.remove_player(account_id);
            if level_id != 0 {
                pm.remove_from_level(level_id, account_id);
            }
        });

        self.game_server.state.room_manager.with_any(packet.room_id, |pm| {
            pm.create_player(account_id);
        });

        self.send_packet_static(&RoomJoinedPacket).await
    });

    gs_handler!(self, handle_leave_room, LeaveRoomPacket, _packet, {
        let account_id = gs_needauth!(self);

        let room_id = self.room_id.swap(0, Ordering::Relaxed);
        if room_id == 0 {
            return Ok(());
        }

        let level_id = self.level_id.load(Ordering::Relaxed);

        self.game_server.state.room_manager.with_any(room_id, |pm| {
            pm.remove_player(account_id);
            if level_id != 0 {
                pm.remove_from_level(level_id, account_id);
            }
        });

        // add them to the global room
        self.game_server.state.room_manager.get_global().create_player(account_id);

        // maybe delete the old room if there are no more players there
        self.game_server.state.room_manager.maybe_remove_room(room_id);

        // respond with the global room list
        self._respond_with_room_list(0).await
    });

    gs_handler!(self, handle_request_room_list, RequestRoomPlayerListPacket, _packet, {
        let _ = gs_needauth!(self);

        let room_id = self.room_id.load(Ordering::Relaxed);
        self._respond_with_room_list(room_id).await
    });

    gs_handler!(self, handle_request_level_list, RequestLevelListPacket, _packet, {
        let _ = gs_needauth!(self);

        let room_id = self.room_id.load(Ordering::Relaxed);

        let level_count = self
            .game_server
            .state
            .room_manager
            .with_any(room_id, |room| room.get_level_count());

        let encoded_size = size_of_types!(u32) + size_of_types!(GlobedLevel) * level_count;

        self.send_packet_alloca_with::<LevelListPacket, _>(encoded_size, |buf| {
            self.game_server.state.room_manager.with_any(room_id, |pm| {
                buf.write_list_with(level_count, |buf| {
                    pm.for_each_level(
                        |(level_id, players), count, buf| {
                            // skip editorcollab levels
                            if count < level_count && !is_editorcollab_level(level_id) {
                                buf.write_value(&GlobedLevel {
                                    level_id,
                                    player_count: players.len() as u16,
                                });
                                true
                            } else {
                                false
                            }
                        },
                        buf,
                    )
                });
            });
        })
        .await
    });

    gs_handler!(self, handle_request_player_count, RequestPlayerCountPacket, packet, {
        let _ = gs_needauth!(self);

        let room_id = self.room_id.load(Ordering::Relaxed);

        // request all main levels
        let encoded_size = size_of_types!(u32) + size_of_types!(LevelId, u16) * packet.level_ids.len();

        self.send_packet_alloca_with::<LevelPlayerCountPacket, _>(encoded_size, |buf| {
            self.game_server.state.room_manager.with_any(room_id, |pm| {
                buf.write_list_with(packet.level_ids.len(), |buf| {
                    for level_id in &*packet.level_ids {
                        buf.write_value(level_id);
                        buf.write_u16(pm.get_player_count_on_level(*level_id).unwrap_or(0) as u16);
                    }

                    packet.level_ids.len()
                });
            });
        })
        .await
    });

    #[inline]
    async fn _respond_with_room_list(&self, room_id: u32) -> crate::server_thread::Result<()> {
        let player_count = self
            .game_server
            .state
            .room_manager
            .with_any(room_id, |room| room.get_total_player_count());

        let encoded_size = size_of_types!(u32, u32) + size_of_types!(PlayerRoomPreviewAccountData) * player_count;

        self.send_packet_alloca_with::<RoomPlayerListPacket, _>(encoded_size, |buf| {
            buf.write_u32(room_id);
            buf.write_list_with(player_count, |buf| {
                self.game_server.for_every_room_player_preview(
                    room_id,
                    move |preview, count, buf| {
                        // we do additional length check because player count may have increased since then
                        if count < player_count {
                            buf.write_value(preview);
                            true
                        } else {
                            false
                        }
                    },
                    buf,
                )
            });
        })
        .await
    }
}
