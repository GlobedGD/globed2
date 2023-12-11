use std::sync::{atomic::Ordering, Arc};

use crate::{
    data::packets::PacketHeader,
    server_thread::{GameServerThread, PacketHandlingError, Result},
};

use super::*;
use crate::data::*;

/// max voice throughput in kb/s
pub const MAX_VOICE_THROUGHPUT: usize = 8;
/// max voice packet size in bytes
pub const MAX_VOICE_PACKET_SIZE: usize = 4096;

impl GameServerThread {
    gs_handler!(self, handle_level_join, LevelJoinPacket, packet, {
        let account_id = gs_needauth!(self);

        let old_level = self.level_id.swap(packet.level_id, Ordering::Relaxed);
        let room_id = self.room_id.load(Ordering::Relaxed);

        self.game_server.state.room_manager.with_any(room_id, |pm| {
            if old_level != 0 {
                pm.remove_from_level(old_level, account_id);
            }

            pm.add_to_level(packet.level_id, account_id);
        });

        Ok(())
    });

    gs_handler!(self, handle_level_leave, LevelLeavePacket, _packet, {
        let account_id = gs_needauth!(self);

        let level_id = self.level_id.swap(0, Ordering::Relaxed);
        if level_id != 0 {
            let room_id = self.room_id.load(Ordering::Relaxed);

            self.game_server.state.room_manager.with_any(room_id, |pm| {
                pm.remove_from_level(level_id, account_id);
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
            pm.set_player_data(account_id, &packet.data);
            // this unwrap should be safe and > 0 given that self.level_id != 0, but we leave a default just in case
            pm.get_player_count_on_level(level_id).unwrap_or(1) - 1
        });

        // no one else on the level, no need to send a response packet
        if written_players == 0 {
            return Ok(());
        }

        let calc_size = size_of_types!(PacketHeader, u32) + size_of_types!(AssociatedPlayerData) * written_players;

        gs_inline_encode!(self, calc_size, buf, {
            buf.write_packet_header::<LevelDataPacket>();
            buf.write_u32(written_players as u32);

            // this is very scary
            let written = self.game_server.state.room_manager.with_any(room_id, |pm| {
                pm.for_each_player_on_level(
                    level_id,
                    |player, count, buf| {
                        // we do additional length check because player count may have increased since 1st lock
                        if count < written_players && player.data.account_id != account_id {
                            buf.write_value(&player.data);
                            true
                        } else {
                            false
                        }
                    },
                    &mut buf,
                )
            });

            // if the player count has instead decreased, we now lied and the client will fail decoding. re-encode the actual count.
            if written != written_players {
                buf.set_pos(PacketHeader::SIZE);
                buf.write_u32(written as u32);
            }
        });

        Ok(())
    });

    gs_handler!(self, handle_sync_player_metadata, SyncPlayerMetadataPacket, packet, {
        let account_id = gs_needauth!(self);

        let level_id = self.level_id.load(Ordering::Relaxed);
        if level_id == 0 {
            return Err(PacketHandlingError::UnexpectedPlayerData);
        }

        let room_id = self.room_id.load(Ordering::Relaxed);

        let total_players = self.game_server.state.room_manager.with_any(room_id, |pm| {
            pm.set_player_metadata(account_id, &packet.data);
            // this unwrap should be safe and > 0 given that self.level_id != 0, but we leave a default just in case
            pm.get_player_count_on_level(level_id).unwrap_or(1) - 1
        });

        // no one else on the level, no need to send a response packet
        if total_players == 0 {
            return Ok(());
        }

        let written_players = if packet.requested.is_some() { 1 } else { total_players };

        let calc_size = size_of_types!(PacketHeader, u32) + size_of_types!(FullPlayerMetadata) * written_players;

        gs_inline_encode!(self, calc_size, buf, {
            buf.write_packet_header::<PlayerMetadataPacket>();
            buf.write_u32(written_players as u32);

            let written = self.game_server.state.room_manager.with_any(room_id, |pm| {
                // if they requested one specific player, encode just them (if we find them)
                if let Some(player_id) = packet.requested {
                    if let Some(meta) = pm.get_player_metadata(player_id) {
                        let account_data = self.game_server.get_player_account_data(player_id);
                        if account_data.is_some() {
                            let fpm = FullPlayerMetadata {
                                metadata: *meta,
                                account_data: account_data.unwrap(),
                            };

                            buf.write_value(&fpm);
                            return 1;
                        }
                    }

                    return 0;
                }

                // otherwise, encode everyone on the level
                pm.for_each_player_on_level(
                    level_id,
                    |player, count, buf| {
                        let account_data = self.game_server.get_player_account_data(player.data.account_id);
                        // we do additional length check because player count may have changed since 1st lock
                        if count < written_players && player.data.account_id != account_id && account_data.is_some() {
                            let fpm = FullPlayerMetadata {
                                metadata: player.meta,
                                account_data: account_data.unwrap(),
                            };

                            buf.write_value(&fpm);
                            true
                        } else {
                            false
                        }
                    },
                    &mut buf,
                )
            });

            // if the player count has instead decreased, we now lied and the client will fail decoding. re-encode the actual count.
            if written != written_players {
                buf.set_pos(PacketHeader::SIZE);
                buf.write_u32(written as u32);
            }
        });

        Ok(())
    });

    gs_handler!(self, handle_voice, VoicePacket, packet, {
        let account_id = gs_needauth!(self);

        let vpkt = Arc::new(VoiceBroadcastPacket {
            player_id: account_id,
            data: packet.data,
        });

        self.game_server.broadcast_voice_packet(
            &vpkt,
            self.level_id.load(Ordering::Relaxed),
            self.room_id.load(Ordering::Relaxed),
        )?;

        Ok(())
    });

    gs_handler!(self, handle_chat_message, ChatMessagePacket, packet, {
        let account_id = gs_needauth!(self);

        let cpkt = ChatMessageBroadcastPacket {
            player_id: account_id,
            message: packet.message,
        };

        self.game_server.broadcast_chat_packet(
            &cpkt,
            self.level_id.load(Ordering::Relaxed),
            self.room_id.load(Ordering::Relaxed),
        )?;

        Ok(())
    });
}
