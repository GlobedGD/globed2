use std::{
    sync::{atomic::Ordering, Arc},
    time::{SystemTime, UNIX_EPOCH},
};

use crate::{
    data::packets::PacketHeader,
    server_thread::{GameServerThread, PacketHandlingError, Result},
};
use log::{debug, warn};

use super::{gs_disconnect, gs_handler, gs_needauth};
use crate::data::*;

/// max voice throughput in kb/s
const MAX_VOICE_THROUGHPUT: usize = 8;
/// max voice packet size in bytes
pub const MAX_VOICE_PACKET_SIZE: usize = 4096;

impl GameServerThread {
    gs_handler!(self, handle_sync_icons, SyncIconsPacket, packet, {
        gs_needauth!(self);

        self.account_data.lock().icons.clone_from(&packet.icons);
        Ok(())
    });

    gs_handler!(self, handle_request_profiles, RequestProfilesPacket, packet, {
        gs_needauth!(self);

        let profiles = self.game_server.gather_profiles(&packet.ids);
        let encoded_size = size_of_types!(PlayerAccountData) * profiles.len();

        self.send_packet_fast_rough(&PlayerProfilesPacket { profiles }, encoded_size)
            .await
    });

    gs_handler!(self, handle_level_join, LevelJoinPacket, packet, {
        gs_needauth!(self);

        let account_id = self.account_id.load(Ordering::Relaxed);
        let old_level = self.level_id.swap(packet.level_id, Ordering::Relaxed);

        let mut pm = self.game_server.state.player_manager.lock();

        if old_level != 0 {
            pm.remove_from_level(old_level, account_id);
        }

        pm.add_to_level(packet.level_id, account_id);

        Ok(())
    });

    gs_handler!(self, handle_level_leave, LevelLeavePacket, _packet, {
        gs_needauth!(self);

        let level_id = self.level_id.load(Ordering::Relaxed);
        if level_id != 0 {
            let account_id = self.account_id.load(Ordering::Relaxed);

            let mut pm = self.game_server.state.player_manager.lock();
            pm.remove_from_level(level_id, account_id);
        }

        Ok(())
    });

    // if you are seeing this. i am so sorry.
    gs_handler!(self, handle_player_data, PlayerDataPacket, packet, {
        gs_needauth!(self);

        let level_id = self.level_id.load(Ordering::Relaxed);
        if level_id == 0 {
            return Err(PacketHandlingError::UnexpectedPlayerData);
        }

        let account_id = self.account_id.load(Ordering::Relaxed);

        let retval: Result<Option<Vec<u8>>> = {
            let mut pm = self.game_server.state.player_manager.lock();
            pm.set_player_data(account_id, &packet.data);

            // this unwrap should be safe and > 0 given that self.level_id != 0
            let written_players = pm.get_player_count_on_level(level_id).unwrap() - 1;

            // no one else on the level, no need to send a response packet
            if written_players == 0 {
                return Ok(());
            }

            drop(pm);

            let calc_size = size_of_types!(PacketHeader, u32) + size_of_types!(AssociatedPlayerData) * written_players;

            alloca::with_alloca(calc_size, move |data| {
                // safety: 'data' will have garbage data but that is considered safe for all our intents and purposes
                // as `FastByteBuffer::as_bytes()` will only return what was already written.
                let data = unsafe {
                    let ptr = data.as_mut_ptr().cast::<u8>();
                    let len = std::mem::size_of_val(data);
                    std::slice::from_raw_parts_mut(ptr, len)
                };

                let mut buf = FastByteBuffer::new(data);

                // dont actually do this anywhere else please
                buf.write_packet_header::<LevelDataPacket>();
                buf.write_u32(written_players as u32);

                // this is scary
                self.game_server.state.player_manager.lock().for_each_player_on_level(
                    level_id,
                    |player, buf| {
                        // we do additional length check because player count may have changed since 1st lock
                        // NOTE: this assumes encoded size of AssociatedPlayerData is a constant
                        // change this to something else if that won't be true in the future
                        if buf.len() != buf.capacity() && player.account_id != account_id {
                            buf.write_value(player);
                        }
                    },
                    &mut buf,
                );

                let data = buf.as_bytes();
                match self.send_buffer_immediate(data) {
                    // if we cant send without blocking, accept our defeat and clone the data to a vec
                    Err(PacketHandlingError::SocketWouldBlock) => Ok(Some(data.to_vec())),
                    // if another error occured, propagate it up
                    Err(e) => Err(e),
                    // if all good, do nothing
                    Ok(()) => Ok(None),
                }
            })
        };

        if let Some(data) = retval? {
            debug!("fast PlayerData response failed, issuing a blocking call");
            self.send_buffer(&data).await?;
        }

        Ok(())
    });

    gs_handler!(self, handle_request_player_list, RequestPlayerListPacket, _packet, {
        gs_needauth!(self);

        let profiles = self.game_server.gather_all_profiles();
        let encoded_size = size_of_types!(PlayerAccountData) * profiles.len();

        self.send_packet_fast_rough(&PlayerListPacket { profiles }, encoded_size)
            .await
    });

    gs_handler!(self, handle_voice, VoicePacket, packet, {
        gs_needauth!(self);

        let accid = self.account_id.load(Ordering::Relaxed);

        // check the throughput
        {
            let now = SystemTime::now().duration_since(UNIX_EPOCH)?.as_millis() as u64;
            let last_voice_packet = self.last_voice_packet.swap(now, Ordering::Relaxed);
            let mut passed_time = now - last_voice_packet;

            if passed_time == 0 {
                passed_time = 1;
            }

            let total_size = packet.data.data.len();

            // let total_size = packet
            //     .data
            //     .opus_frames
            //     .iter()
            //     .filter_map(|opt| opt.as_ref())
            //     .map(Vec::len)
            //     .sum::<usize>();

            let throughput = total_size / passed_time as usize; // in kb per second

            if throughput > MAX_VOICE_THROUGHPUT {
                warn!("rejecting a voice packet, throughput above the limit: {}kb/s", throughput);
                return Ok(());
            }
        }

        let vpkt = Arc::new(VoiceBroadcastPacket {
            player_id: accid,
            data: packet.data,
        });

        self.game_server
            .broadcast_voice_packet(&vpkt, self.level_id.load(Ordering::Relaxed))?;

        Ok(())
    });

    gs_handler!(self, handle_chat_message, ChatMessagePacket, packet, {
        gs_needauth!(self);

        let accid = self.account_id.load(Ordering::Relaxed);

        let cpkt = ChatMessageBroadcastPacket {
            player_id: accid,
            message: packet.message,
        };

        self.game_server
            .broadcast_chat_packet(&cpkt, self.level_id.load(Ordering::Relaxed))?;

        Ok(())
    });
}
