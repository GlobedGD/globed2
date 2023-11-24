use std::{
    sync::{atomic::Ordering, Arc},
    time::SystemTime,
};

use crate::{
    data::packets::PacketHeader,
    server_thread::{GameServerThread, PacketHandlingError, Result},
};
use log::{debug, warn};

use super::{gs_disconnect, gs_handler, gs_needauth};
use crate::data::*;

impl GameServerThread {
    gs_handler!(self, handle_sync_icons, SyncIconsPacket, packet, {
        gs_needauth!(self);

        let mut account_data = self.account_data.lock();
        account_data.icons.clone_from(&packet.icons);
        Ok(())
    });

    gs_handler!(self, handle_request_profiles, RequestProfilesPacket, packet, {
        gs_needauth!(self);

        self.send_packet(&PlayerProfilesPacket {
            profiles: self.game_server.gather_profiles(&packet.ids),
        })
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

            let calc_size = PacketHeader::SIZE + 4 + (written_players * AssociatedPlayerData::ENCODED_SIZE);

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

        self.send_packet(&PlayerListPacket {
            profiles: self.game_server.gather_all_profiles(),
        })
        .await
    });

    gs_handler!(self, handle_voice, VoicePacket, packet, {
        gs_needauth!(self);

        let accid = self.account_id.load(Ordering::Relaxed);

        // check the throughput
        {
            let mut last_voice_packet = self.last_voice_packet.lock();
            let now = SystemTime::now();
            let passed_time = now.duration_since(*last_voice_packet)?.as_millis();
            *last_voice_packet = now;

            let total_size = packet.data.opus_frames.iter().map(Vec::len).sum::<usize>();

            let throughput = total_size / passed_time as usize; // in kb/s

            debug!("voice packet throughput: {}kb/s", throughput);
            if throughput > 8 {
                warn!("rejecting a voice packet, throughput above the limit: {}kb/s", throughput);
                return Ok(());
            }
        }

        let vpkt = Arc::new(VoiceBroadcastPacket {
            player_id: accid,
            data: packet.data.clone(),
        });

        self.game_server.broadcast_voice_packet(&vpkt)?;

        Ok(())
    });
}
