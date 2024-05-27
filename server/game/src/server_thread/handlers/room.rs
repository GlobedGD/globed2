use super::*;
use crate::{data::*, server_thread::GameServerThread, server_thread::ServerThreadMessage};
use globed_shared::{debug, warn};

impl GameServerThread {
    gs_handler!(self, handle_create_room, CreateRoomPacket, packet, {
        let account_id = gs_needauth!(self);

        let room_id: u32 = self.room_id.load(Ordering::Relaxed);

        // if we are already in a room, just return the same room info, otherwise create a new one
        let room_info = if room_id == 0 {
            let room_info = self
                .game_server
                .state
                .room_manager
                .create_room(account_id, packet.room_name, packet.password, packet.settings);

            self.room_id.store(room_info.id, Ordering::Relaxed);
            room_info
        } else {
            let room_info = self.game_server.state.room_manager.get_room_info(room_id);

            if let Some(room_info) = room_info {
                room_info
            } else {
                // somehow in an invalid room? shouldn't happen
                warn!("player is in an invalid room: {}", room_id);
                self.room_id.store(0, Ordering::Relaxed);
                return Ok(());
            }
        };

        self.send_packet_static(&RoomCreatedPacket { info: room_info }).await
    });

    gs_handler!(self, handle_join_room, JoinRoomPacket, packet, {
        let account_id = gs_needauth!(self);

        if !self.game_server.state.room_manager.is_valid_room(packet.room_id) {
            return self
                .send_packet_static(&RoomJoinFailedPacket {
                    was_invalid: true,
                    ..Default::default()
                })
                .await;
        }

        // check if we are even able to join the room
        let (was_invalid, was_protected, was_full) = self.game_server.state.room_manager.try_with_any(
            packet.room_id,
            |room| {
                if !room.verify_password(&packet.password) {
                    (false, true, false)
                } else if room.is_full() {
                    (false, false, true)
                } else {
                    (false, false, false)
                }
            },
            || (true, false, false),
        );

        if was_invalid || was_protected || was_full {
            return self
                .send_packet_static(&RoomJoinFailedPacket {
                    was_invalid,
                    was_protected,
                    was_full,
                })
                .await;
        }

        let old_room_id = self.room_id.swap(packet.room_id, Ordering::Relaxed);

        // if we somehow tried to join the same room, do nothing
        if old_room_id == packet.room_id {
            return Ok(());
        }

        let level_id = self.level_id.load(Ordering::Relaxed);

        // remove the player from the previously connected room (or the global room)
        self.game_server.state.room_manager.remove_with_any(old_room_id, account_id, level_id);

        self.game_server.state.room_manager.with_any(packet.room_id, |pm| {
            pm.manager.create_player(account_id);
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

        let should_send_update = self.game_server.state.room_manager.remove_with_any(room_id, account_id, level_id);

        // if we were the owner, send update packets to everyone
        if should_send_update {
            self.game_server.broadcast_room_info(room_id).await;
        }

        // add them to the global room
        self.game_server.state.room_manager.get_global().manager.create_player(account_id);

        // respond with the global room list
        self._respond_with_room_list(0).await
    });

    gs_handler!(self, handle_request_room_players, RequestRoomPlayerListPacket, _packet, {
        let _ = gs_needauth!(self);

        let room_id = self.room_id.load(Ordering::Relaxed);
        self._respond_with_room_list(room_id).await
    });

    gs_handler!(self, handle_update_room_settings, UpdateRoomSettingsPacket, packet, {
        let account_id = gs_needauth!(self);

        let room_id = self.room_id.load(Ordering::Relaxed);

        if room_id == 0 {
            return Ok(());
        }

        let mut success = false;

        self.game_server.state.room_manager.with_any(room_id, |room| {
            if room.owner == account_id {
                room.set_settings(&packet.settings);
                success = true;
            }
        });

        // send an update packet to all clients
        if success {
            self.game_server.broadcast_room_info(room_id).await;
        }

        Ok(())
    });

    gs_handler!(self, handle_room_invitation, RoomSendInvitePacket, packet, {
        let account_id = gs_needauth!(self);

        let room_id = self.room_id.load(Ordering::Relaxed);

        // if we are in global room, skip
        if room_id == 0 {
            return Ok(());
        }

        // if we don't have permission to invite, skip
        let room_password = self.game_server.state.room_manager.with_any(room_id, |room| {
            if room.is_protected() && (room.is_public_invites() || room.owner == account_id) {
                Some(room.password.clone())
            } else if room.is_protected() {
                None
            } else {
                Some(InlineString::default())
            }
        });

        let Some(room_password) = room_password else {
            #[cfg(debug_assertions)]
            debug!("invite from {account_id} rejected, user is unable to invite");
            return Ok(());
        };

        let thread = self.game_server.get_user_by_id(packet.player);

        if let Some(thread) = thread {
            let player_data = self.account_data.lock().make_preview();

            debug!(
                "{account_id} sent an invite to {} (room: {}, password: {})",
                packet.player, room_id, room_password
            );

            let invite_packet = RoomInvitePacket {
                player_data,
                room_id,
                room_password,
            };

            thread.push_new_message(ServerThreadMessage::BroadcastInvite(invite_packet.clone())).await;
        }

        Ok(())
    });

    gs_handler!(self, handle_request_room_list, RequestRoomListPacket, _packet, {
        let _ = gs_needauth!(self);

        self._respond_with_list_of_rooms().await
    });

    #[inline]
    async fn _respond_with_room_list(&self, room_id: u32) -> crate::server_thread::Result<()> {
        let (player_count, room_info) = self.game_server.state.room_manager.with_any(room_id, |room| {
            (room.manager.get_total_player_count(), room.get_room_info(room_id, self.game_server))
        });

        // RoomInfo, list size, list of PlayerRoomPreviewAccountData
        let encoded_size = size_of_types!(RoomInfo, u32) + size_of_types!(PlayerRoomPreviewAccountData) * player_count;

        self.send_packet_alloca_with::<RoomPlayerListPacket, _>(encoded_size, |buf| {
            buf.write_value(&room_info);
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

    #[inline]
    async fn _respond_with_list_of_rooms(&self) -> crate::server_thread::Result<()> {
        let room_count = self
            .game_server
            .state
            .room_manager
            .get_rooms()
            .iter()
            .filter(|(_, room)| !room.is_hidden())
            .count();

        let encoded_size = size_of_types!(u32) + size_of_types!(RoomListingInfo) * room_count;

        self.send_packet_alloca_with::<RoomListPacket, _>(encoded_size, |buf| {
            buf.write_list_with(room_count, |buf| {
                self.game_server.for_every_public_room(
                    move |room, count, buf| {
                        if count < room_count {
                            buf.write_value(&room);
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
