use std::sync::atomic::Ordering;

use crate::{
    data::packets::PacketHeader,
    server_thread::{GameServerThread, PacketHandlingError, Result},
};

use super::*;
use crate::data::*;

impl GameServerThread {
    gs_handler!(self, handle_sync_icons, SyncIconsPacket, packet, {
        let _ = gs_needauth!(self);

        self.account_data.lock().icons.clone_from(&packet.icons);
        Ok(())
    });

    gs_handler!(self, handle_request_global_list, RequestGlobalPlayerListPacket, _packet, {
        let _ = gs_needauth!(self);

        let player_count = self.game_server.state.player_count.load(Ordering::Relaxed);
        let encoded_size =
            size_of_types!(PacketHeader, u32) + size_of_types!(PlayerPreviewAccountData) * player_count as usize;

        gs_inline_encode!(self, encoded_size, buf, {
            buf.write_packet_header::<GlobalPlayerListPacket>();
            buf.write_u32(player_count);

            let written = self.game_server.for_every_player_preview(
                move |preview, count, buf| {
                    // we do additional length check because player count may have increased since then
                    if count < player_count as usize {
                        buf.write_value(preview);
                        true
                    } else {
                        false
                    }
                },
                &mut buf,
            ) as u32;

            // if the player count has instead decreased, we now lied and the client will fail decoding. re-encode the actual count.
            if written != player_count {
                buf.set_pos(PacketHeader::SIZE);
                buf.write_u32(written);
            }
        });

        Ok(())
    });

    gs_handler!(self, handle_create_room, CreateRoomPacket, _packet, {
        let account_id = gs_needauth!(self);

        // if we are already in a room, just return the same room ID, otherwise create a new one
        let mut room_id: u32 = self.room_id.load(Ordering::Relaxed);

        if room_id == 0 {
            room_id = self.game_server.state.room_manager.create_room(account_id);
            self.room_id.store(room_id, Ordering::Relaxed);
        }

        self.send_packet_fast(&RoomCreatedPacket { room_id }).await
    });

    gs_handler!(self, handle_join_room, JoinRoomPacket, packet, {
        let account_id = gs_needauth!(self);

        if !self.game_server.state.room_manager.is_valid_room(packet.room_id) {
            return self.send_packet_fast(&RoomJoinFailedPacket).await;
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

        self.send_packet_fast(&RoomJoinedPacket).await
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

        Ok(())
    });

    gs_handler!(self, handle_request_room_list, RequestRoomPlayerListPacket, _packet, {
        let _ = gs_needauth!(self);

        let room_id = self.room_id.load(Ordering::Relaxed);

        let player_count = self
            .game_server
            .state
            .room_manager
            .with_any(room_id, |room| room.get_total_player_count());

        let encoded_size = size_of_types!(PacketHeader, u32) + size_of_types!(PlayerRoomPreviewAccountData) * player_count;

        gs_inline_encode!(self, encoded_size, buf, {
            buf.write_packet_header::<RoomPlayerListPacket>();
            buf.write_u32(player_count as u32);

            let written = self.game_server.for_every_room_player_preview(
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
                &mut buf,
            );

            // if the player count has instead decreased, we now lied and the client will fail decoding. re-encode the actual count.
            if written != player_count {
                buf.set_pos(PacketHeader::SIZE);
                buf.write_u32(written as u32);
            }
        });

        Ok(())
    });
}
