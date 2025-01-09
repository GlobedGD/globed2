use crate::webhook::{WebhookChannel, WebhookMessage};
use std::borrow::Cow;

use super::*;

impl ClientThread {
    gs_handler!(self, handle_create_room, CreateRoomPacket, packet, {
        let account_id = gs_needauth!(self);

        let room_id: u32 = self.room_id.load(Ordering::Relaxed);

        // if we are already in a room, just return the same room info, otherwise create a new one
        let room_info = if room_id == 0 {
            // check if data is valid

            let fail_reason: Option<&'static str> = match packet.room_name.to_str() {
                Ok(str) => {
                    if self.game_server.state.filter.is_bad(str) {
                        Some("Inappropriate room name. Please note that trying to bypass the filter may lead to a ban.")
                    } else {
                        None
                    }
                }
                Err(_) => Some("invalid room name"),
            };

            if let Some(reason) = fail_reason {
                return self
                    .send_packet_dynamic(&RoomCreateFailedPacket {
                        reason: Cow::Borrowed(reason),
                    })
                    .await;
            }

            let room = self
                .game_server
                .state
                .room_manager
                .create_room(account_id, packet.room_name, packet.password, packet.settings);

            self.room_id.store(room.id, Ordering::Relaxed);
            *self.room.lock() = room;

            let self_name = self.account_data.lock().name.try_to_string();

            let room_info = self.room.lock().get_room_info();

            if self.game_server.bridge.has_room_webhook() {
                if let Err(err) = self
                    .game_server
                    .bridge
                    .send_webhook_messages(
                        &[WebhookMessage::RoomCreated(
                            room_info.id,
                            room_info.name.try_to_string(),
                            self_name,
                            account_id,
                            packet.settings.flags.is_hidden,
                            !room_info.password.is_empty(),
                        )],
                        WebhookChannel::Room,
                    )
                    .await
                {
                    warn!("Failed to send webhook message (room creation): {err}");
                }
            }

            room_info
        } else {
            self.room.lock().get_room_info()
        };

        info!(
            "Room created: {} ({}) (by {} ({}))",
            room_info.name, room_info.id, room_info.owner.name, room_info.owner.account_id
        );

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
        self.game_server.state.room_manager.remove_player(&self.room.lock(), account_id, level_id);

        // set the current room
        let room = {
            let mut room = self.room.lock();
            *room = self.game_server.state.room_manager.get_room_or_global(packet.room_id);

            let mut manager = room.manager.write();

            let is_invisible = self.privacy_settings.lock().get_hide_in_game();
            manager.create_player(account_id, is_invisible);

            // if we are in any level, clean transition to there
            if level_id != 0 {
                manager.add_to_level(level_id, account_id, self.on_unlisted_level.load(Ordering::SeqCst));
            }

            room.clone()
        };

        self._respond_with_room_list(room, true).await
    });

    gs_handler!(self, handle_leave_room, LeaveRoomPacket, _packet, {
        let _ = gs_needauth!(self);

        self._remove_from_room().await;

        // respond with the global room list
        self._respond_with_room_list(self.game_server.state.room_manager.get_global_owned(), false)
            .await
    });

    gs_handler!(self, handle_request_room_players, RequestRoomPlayerListPacket, _packet, {
        let _ = gs_needauth!(self);

        let room = self.room.lock().clone();

        self._respond_with_room_list(room, false).await
    });

    gs_handler!(self, handle_update_room_settings, UpdateRoomSettingsPacket, packet, {
        let account_id = gs_needauth!(self);

        if !self.is_in_room() {
            return Ok(());
        }

        let room = self.room.lock().clone();

        if room.owner.load(Ordering::Relaxed) == account_id {
            room.set_settings(packet.settings);
            // send an update packet to all clients
            self.game_server.broadcast_room_info(room).await;
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
        let room_password = {
            let room = self.room.lock();

            if room.is_protected() && (room.is_public_invites() || room.get_owner() == account_id) {
                Some(room.password.clone())
            } else if room.is_protected() {
                None
            } else {
                Some(InlineString::default())
            }
        };

        let Some(room_password) = room_password else {
            #[cfg(debug_assertions)]
            debug!("invite from {account_id} rejected, user is unable to invite");
            return Ok(());
        };

        let thread = self.game_server.get_user_by_id(packet.player);

        if let Some(thread) = thread {
            let player_data = self.account_data.lock().make_preview(!self.privacy_settings.lock().get_hide_roles());

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

        let pkt = RoomListPacket {
            rooms: self
                .game_server
                .state
                .room_manager
                .get_rooms()
                .iter()
                .filter(|(_, room)| !room.is_hidden())
                .map(|(id, room)| room.get_room_listing_info(*id))
                .collect(),
        };

        self.send_packet_dynamic(&pkt).await
    });

    gs_handler!(self, handle_close_room, CloseRoomPacket, packet, {
        let account_id = gs_needauth!(self);

        // use the provided room id or the current room if 0
        let mut room_id = packet.room_id;
        if room_id == 0 {
            room_id = self.room_id.load(Ordering::Relaxed);
        }

        if room_id == 0 {
            let room = self.room.lock().clone();
            return self._respond_with_room_list(room, false).await;
        }

        let players = self.game_server.state.room_manager.try_with_any(
            room_id,
            |room| {
                // we can only close a room if we are the creator or if we are a mod
                if room.get_owner() != account_id && !self.can_moderate() {
                    return Vec::new();
                }

                let mut v = Vec::new();
                room.manager.read().for_each_player(|player| {
                    if player.account_id != account_id {
                        v.push(player.account_id);
                    }
                });

                v
            },
            Vec::new,
        );

        for player in players {
            self.game_server.broadcast_room_kicked(player).await;
        }

        self._kicked_from_room().await
    });

    gs_handler!(self, handle_kick_room_player, KickRoomPlayerPacket, packet, {
        let _ = gs_needauth!(self);

        if !self.is_in_room() {
            return Ok(());
        }

        // check if the player is in our room
        let has_player = self.room.lock().has_player(packet.player);

        if has_player {
            self.game_server.broadcast_room_kicked(packet.player).await;
        }

        Ok(())
    });

    pub async fn _kicked_from_room(&self) -> crate::client::Result<()> {
        self._remove_from_room().await;

        let room = self.room.lock().clone();
        // respond with the global room list
        self._respond_with_room_list(room, false).await
    }

    async fn _remove_from_room(&self) {
        let account_id = self.account_id.load(Ordering::Relaxed);

        let room_id = self.room_id.swap(0, Ordering::Relaxed);
        if room_id == 0 {
            return;
        }

        let level_id = self.level_id.load(Ordering::Relaxed);

        let should_send_update = self.game_server.state.room_manager.remove_player(&self.room.lock(), account_id, level_id);

        // if we were the owner, send update packets to everyone
        if should_send_update {
            let room = self.room.lock().clone();
            self.game_server.broadcast_room_info(room).await;
        }

        let is_invisible = self.privacy_settings.lock().get_hide_in_game();

        // add them to the global room
        self.game_server
            .state
            .room_manager
            .get_global()
            .manager
            .write()
            .create_player(account_id, is_invisible);

        // set the current room to global
        *self.room.lock() = self.game_server.state.room_manager.get_global_owned();
    }

    #[inline]
    async fn _respond_with_room_list(&self, room: Arc<Room>, just_joined: bool) -> Result<()> {
        let room_info = room.get_room_info();

        let players = self
            .game_server
            .get_room_player_previews(&room, self.account_id.load(Ordering::Relaxed), self.can_moderate());

        if just_joined {
            self.send_packet_dynamic(&RoomJoinedPacket { room_info, players }).await
        } else {
            self.send_packet_dynamic(&RoomPlayerListPacket { room_info, players }).await
        }
    }
}
