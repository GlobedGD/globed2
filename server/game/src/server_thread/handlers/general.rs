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

    gs_handler!(self, handle_request_level_list, RequestLevelListPacket, _packet, {
        let _ = gs_needauth!(self);

        let room_id = self.room_id.load(Ordering::Relaxed);

        let level_count = self
            .game_server
            .state
            .room_manager
            .with_any(room_id, |room| room.manager.get_level_count());

        let encoded_size = size_of_types!(u32) + size_of_types!(GlobedLevel) * level_count;

        self.send_packet_alloca_with::<LevelListPacket, _>(encoded_size, |buf| {
            self.game_server.state.room_manager.with_any(room_id, |pm| {
                buf.write_list_with(level_count, |buf| {
                    pm.manager.for_each_level(
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
                        buf.write_u16(pm.manager.get_player_count_on_level(*level_id).unwrap_or(0) as u16);
                    }

                    packet.level_ids.len()
                });
            });
        })
        .await
    });
}
