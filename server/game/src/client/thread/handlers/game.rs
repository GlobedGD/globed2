use std::sync::{Arc, atomic::Ordering};

use super::*;

/// max voice packet size in bytes
pub const MAX_VOICE_PACKET_SIZE: usize = 4096;

impl ClientThread {
    gs_handler!(self, handle_level_join, LevelJoinPacket, packet, {
        self._handle_level_join(packet.level_id, packet.unlisted).await
    });

    async fn _handle_level_join(&self, level_id: LevelId, unlisted: bool) -> crate::client::Result<()> {
        let account_id = gs_needauth!(self);

        self.on_unlisted_level.store(unlisted, Ordering::SeqCst);

        let old_level = self.level_id.swap(level_id, Ordering::Relaxed);

        let room = self.room.lock();

        let mut manager = room.manager.write();

        if old_level != 0 {
            manager.remove_from_level(old_level, account_id);
        }

        if level_id != 0 {
            manager.add_to_level(level_id, account_id, unlisted);
        }

        Ok(())
    }

    gs_handler!(self, handle_level_leave, LevelLeavePacket, _packet, {
        let account_id = gs_needauth!(self);

        let level_id = self.level_id.swap(0, Ordering::Relaxed);
        if level_id != 0 {
            self.room.lock().manager.write().remove_from_level(level_id, account_id);
        }

        Ok(())
    });

    gs_handler!(self, handle_player_data, PlayerDataPacket, packet, {
        let account_id = gs_needauth!(self);

        let level_id = self.level_id.load(Ordering::Relaxed);
        if level_id == 0 {
            return Err(PacketHandlingError::UnexpectedPlayerData);
        }

        let is_mod = self.can_moderate();

        let (written_players, metadatas, estimated_size) = {
            let room = self.room.lock();

            let mut manager = room.manager.write();
            // set metadata
            manager.set_player_data(account_id, &packet.data);

            // run custom item id changes
            if !packet.counter_changes.is_empty() {
                manager.run_counter_actions_on_level(level_id, &packet.counter_changes);
            }

            // this unwrap should be safe and > 0 given that self.level_id != 0, but we leave a default just in case
            let player_count = manager.get_player_count_on_level(level_id).unwrap_or(1) - 1;

            // retrieve metadata of other players, if was asked
            let mut metavec = Vec::new();
            let mut estimated_size = 0usize;
            let mut should_add_meta = false;

            if let Some(meta) = packet.meta {
                manager.set_player_meta(account_id, &meta);
                metavec = Vec::with_capacity(player_count);
                should_add_meta = true;
            }

            manager.for_each_player_on_level(level_id, |player| {
                if player.account_id != account_id && (!player.is_invisible || is_mod) {
                    if should_add_meta {
                        metavec.push(AssociatedPlayerMetadata {
                            account_id: player.account_id,
                            data: player.meta.clone(),
                        });
                    }

                    estimated_size += player.data.encoded_size() + size_of_types!(i32);
                }
            });

            // add the size of the custom items as well
            if let Some(level) = manager.get_level(level_id) {
                estimated_size += level.custom_items.encoded_size();
            }

            (player_count, metavec, estimated_size)
        };

        // no one else on the level, if no item ids changed there is no need to send a response packet
        if written_players == 0 {
            // TODO aah idk
            // if !packet.counter_changes.is_empty() {
            // TODO not dynamic but alloca with custom encoder ig
            // and just make this less sloppy tbh i cba
            let custom_items = self.room.lock().manager.read().get_level(level_id).map(|x| x.custom_items.clone());

            if let Some(custom_items) = custom_items {
                self.send_packet_dynamic::<LevelDataPacket>(&LevelDataPacket {
                    players: Vec::new(),
                    custom_items: Some(custom_items),
                })
                .await?;
            }
            // }

            return Ok(());
        }

        let calc_size = size_of_types!(u32) + estimated_size;

        // 8 is a safety buffer just in case something goes ary
        self.send_packet_alloca_with::<LevelDataPacket, _>(calc_size + written_players * 8 + 64, |buf| {
            let room = self.room.lock();

            let manager = room.manager.read();

            buf.write_list_with(written_players, |buf| {
                let mut count = 0usize;
                manager.for_each_player_on_level(level_id, |player| {
                    if count < written_players && player.account_id != account_id && (!player.is_invisible || is_mod) {
                        buf.write_value(&player.account_id);
                        buf.write_value(&player.data);
                        count += 1;
                    }
                });

                count
            });

            // write custom_items hashmap

            let custom_items = manager.get_level(level_id).map(|x| x.custom_items.clone());

            if custom_items.as_ref().is_some_and(|x| !x.is_empty()) {
                buf.write_bool(true);
                buf.write_value(&custom_items.unwrap());
            } else {
                buf.write_bool(false);
            }
        })
        .await?;

        // send metadata
        if !metadatas.is_empty() {
            self.send_packet_dynamic(&LevelPlayerMetadataPacket { players: metadatas }).await?;
        }

        Ok(())
    });

    gs_handler!(self, handle_request_profiles, RequestPlayerProfilesPacket, packet, {
        let _ = gs_needauth!(self);

        let level_id = self.level_id.load(Ordering::Relaxed);
        if level_id == 0 {
            return Err(PacketHandlingError::UnexpectedPlayerData);
        }

        let is_mod = self.can_moderate();

        // if they requested just one player - use the fast heapless path
        if packet.requested != 0 {
            let calc_size = size_of_types!(VarLength) + size_of_types!(PlayerAccountData);
            let account_data = self.game_server.get_player_account_data(packet.requested, is_mod);

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

        let players = {
            let room = self.room.lock();
            let manager = room.manager.read();

            // this unwrap should be safe and > 0 given that self.level_id != 0, but we leave a default just in case
            let total_players = manager.get_player_count_on_level(level_id).unwrap_or(1) - 1;

            if total_players == 0 {
                Vec::new()
            } else {
                let mut vec = Vec::with_capacity(total_players);
                manager.for_each_player_on_level(level_id, |player| {
                    let account_data = self.game_server.get_player_account_data(player.account_id, is_mod);
                    if let Some(d) = account_data {
                        vec.push(d);
                    }
                });

                vec
            }
        };

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
