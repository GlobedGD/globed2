use std::sync::{atomic::Ordering, Arc};

use super::*;

/// max voice packet size in bytes
pub const MAX_VOICE_PACKET_SIZE: usize = 4096;

impl ClientThread {
    gs_handler!(self, handle_level_join, LevelJoinPacket, packet, {
        let account_id = gs_needauth!(self);

        let old_level = self.level_id.swap(packet.level_id, Ordering::Relaxed);
        let room_id = self.room_id.load(Ordering::Relaxed);

        self.game_server.state.room_manager.with_any(room_id, |pm| {
            if old_level != 0 {
                pm.manager.remove_from_level(old_level, account_id);
            }

            if packet.level_id != 0 {
                pm.manager.add_to_level(packet.level_id, account_id);
            }
        });

        Ok(())
    });

    gs_handler!(self, handle_level_leave, LevelLeavePacket, _packet, {
        let account_id = gs_needauth!(self);

        let level_id = self.level_id.swap(0, Ordering::Relaxed);
        if level_id != 0 {
            let room_id = self.room_id.load(Ordering::Relaxed);

            self.game_server.state.room_manager.with_any(room_id, |pm| {
                pm.manager.remove_from_level(level_id, account_id);
            });
        }

        Ok(())
    });

    gs_handler!(self, handle_player_data, PlayerDataPacket, packet, {
        let account_id = gs_needauth!(self);

        let level_id = self.level_id.load(Ordering::Relaxed);
        if level_id == 0 {
            return Err(PacketHandlingError::UnexpectedPlayerData);
        }

        let room_id = self.room_id.load(Ordering::Relaxed);

        let written_players = self.game_server.state.room_manager.with_any(room_id, |pm| {
            pm.manager.set_player_data(account_id, &packet.data);
            // this unwrap should be safe and > 0 given that self.level_id != 0, but we leave a default just in case
            pm.manager.get_player_count_on_level(level_id).unwrap_or(1) - 1
        });

        // no one else on the level, no need to send a response packet
        if written_players == 0 {
            return Ok(());
        }

        let calc_size = size_of_types!(u32) + size_of_types!(AssociatedPlayerData) * written_players;
        let fragmentation_limit = self.fragmentation_limit.load(Ordering::Relaxed) as usize;

        // if we can fit in one packet, then just send it as-is
        if calc_size <= fragmentation_limit {
            self.send_packet_alloca_with::<LevelDataPacket, _>(calc_size, |buf| {
                self.game_server.state.room_manager.with_any(room_id, |pm| {
                    buf.write_list_with(written_players, |buf| {
                        pm.manager.for_each_player_on_level(
                            level_id,
                            |player, count, buf| {
                                if count < written_players && player.account_id != account_id {
                                    buf.write_value(&player.to_borrowed_associated_data());
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
            .await?;

            return Ok(());
        }

        // get all players into a vec
        let total_fragments = (calc_size + fragmentation_limit - 1) / fragmentation_limit;

        let mut players = Vec::with_capacity(written_players + 4);

        self.game_server.state.room_manager.with_any(room_id, |pm| {
            pm.manager.for_each_player_on_level(
                level_id,
                |player, _, players| {
                    if player.account_id == account_id {
                        false
                    } else {
                        players.push(player.to_associated_data());
                        true
                    }
                },
                &mut players,
            )
        });

        let players_per_fragment = (players.len() + total_fragments - 1) / total_fragments;
        let calc_size = size_of_types!(u32) + size_of_types!(AssociatedPlayerData) * players_per_fragment;

        debug!(
            "sending a fragmented packet (lim: {fragmentation_limit}, per: {players_per_fragment}, frags: {total_fragments}, fragsize: {calc_size})"
        );

        for chunk in players.chunks(players_per_fragment) {
            self.send_packet_alloca_with::<LevelDataPacket, _>(calc_size, |buf| buf.write_value(chunk))
                .await?;
        }

        Ok(())
    });

    gs_handler!(self, handle_player_metadata, PlayerMetadataPacket, packet, {
        let account_id = gs_needauth!(self);

        let level_id = self.level_id.load(Ordering::Relaxed);
        if level_id == 0 {
            return Err(PacketHandlingError::UnexpectedPlayerData);
        }

        let room_id = self.room_id.load(Ordering::Relaxed);
        let players = self.game_server.state.room_manager.with_any(room_id, |pm| {
            pm.manager.set_player_meta(account_id, &packet.data);

            let mut vec = Vec::with_capacity(pm.manager.get_player_count_on_level(level_id).unwrap_or(0));
            pm.manager.for_each_player_on_level(
                level_id,
                |player, _count, vec| {
                    vec.push(AssociatedPlayerMetadata {
                        account_id: player.account_id,
                        data: player.meta.clone(),
                    });
                    true
                },
                &mut vec,
            );

            vec
        });

        if players.is_empty() {
            // if no players, don't send a response packet
            Ok(())
        } else {
            self.send_packet_dynamic(&LevelPlayerMetadataPacket { players }).await
        }
    });

    gs_handler!(self, handle_request_profiles, RequestPlayerProfilesPacket, packet, {
        let _ = gs_needauth!(self);

        let level_id = self.level_id.load(Ordering::Relaxed);
        if level_id == 0 {
            return Err(PacketHandlingError::UnexpectedPlayerData);
        }

        let room_id = self.room_id.load(Ordering::Relaxed);

        // if they requested just one player - use the fast heapless path
        if packet.requested != 0 {
            let calc_size = size_of_types!(VarLength) + size_of_types!(PlayerAccountData);
            let account_data = self.game_server.get_player_account_data(packet.requested);

            if let Some(account_data) = account_data {
                return self
                    .send_packet_alloca_with::<PlayerProfilesPacket, _>(calc_size, |buf| {
                        // write a Vec with length 1
                        buf.write_length(1);
                        buf.write_value(&account_data);
                    })
                    .await;
            }

            return Ok(());
        }

        let players = self.game_server.state.room_manager.with_any(room_id, |pm| {
            // this unwrap should be safe and > 0 given that self.level_id != 0, but we leave a default just in case
            let total_players = pm.manager.get_player_count_on_level(level_id).unwrap_or(1) - 1;

            if total_players == 0 {
                Vec::new()
            } else {
                let mut vec = Vec::with_capacity(total_players);
                pm.manager.for_each_player_on_level(
                    level_id,
                    |player, _count, vec| {
                        let account_data = self.game_server.get_player_account_data(player.account_id);
                        if let Some(d) = account_data {
                            vec.push(d);
                        }

                        true
                    },
                    &mut vec,
                );

                vec
            }
        });

        self.send_packet_dynamic(&PlayerProfilesPacket { players }).await
    });

    /* Note: blocking logic for voice & chat packets is not in here but in the packet receiving function */

    gs_handler!(self, handle_voice, VoicePacket, packet, {
        let account_id = gs_needauth!(self);

        let vpkt = Arc::new(VoiceBroadcastPacket {
            player_id: account_id,
            data: packet.data,
        });

        self.game_server
            .broadcast_voice_packet(&vpkt, self.level_id.load(Ordering::Relaxed), self.room_id.load(Ordering::Relaxed))
            .await;

        Ok(())
    });

    gs_handler!(self, handle_chat_message, ChatMessagePacket, packet, {
        let account_id = gs_needauth!(self);

        if packet.message.is_empty() {
            return Ok(());
        }

        let cpkt = ChatMessageBroadcastPacket {
            player_id: account_id,
            message: packet.message,
        };

        self.game_server
            .broadcast_chat_packet(&cpkt, self.level_id.load(Ordering::Relaxed), self.room_id.load(Ordering::Relaxed))
            .await;

        Ok(())
    });
}
