use super::*;

impl ClientThread {
    gs_handler!(self, handle_sync_icons, SyncIconsPacket, packet, {
        let _ = gs_needauth!(self);

        self.account_data.lock().icons.clone_from(&packet.icons);
        Ok(())
    });

    gs_handler!(self, handle_request_global_list, RequestGlobalPlayerListPacket, _packet, {
        let _ = gs_needauth!(self);

        self.send_packet_dynamic(&GlobalPlayerListPacket {
            players: self.game_server.get_player_previews_in_room(0),
        })
        .await
    });

    gs_handler!(self, handle_request_level_list, RequestLevelListPacket, _packet, {
        let _ = gs_needauth!(self);

        let room_id = self.room_id.load(Ordering::Relaxed);

        let levels = self.game_server.state.room_manager.with_any(room_id, |pm| {
            let mut vec = Vec::with_capacity(pm.manager.get_level_count());

            pm.manager.for_each_level(
                |(level_id, players), _count, vec| {
                    if !is_editorcollab_level(level_id) {
                        vec.push(GlobedLevel {
                            level_id,
                            player_count: players.len() as u16,
                        });
                    }

                    true
                },
                &mut vec,
            );

            vec
        });

        self.send_packet_dynamic(&LevelListPacket { levels }).await
    });

    gs_handler!(self, handle_request_player_count, RequestPlayerCountPacket, packet, {
        let _ = gs_needauth!(self);

        let room_id = self.room_id.load(Ordering::Relaxed);

        let levels = self.game_server.state.room_manager.with_any(room_id, |pm| {
            let mut levels = Vec::with_capacity(0);

            for &level_id in &*packet.level_ids {
                levels.push((level_id, pm.manager.get_player_count_on_level(level_id).unwrap_or(0) as u16));
            }

            levels
        });

        self.send_packet_dynamic(&LevelPlayerCountPacket { levels }).await
    });

    gs_handler!(self, handle_set_player_status, UpdatePlayerStatusPacket, packet, {
        let _ = gs_needauth!(self);

        self.is_invisible.store(packet.is_invisible, Ordering::Relaxed);

        Ok(())
    });
}
