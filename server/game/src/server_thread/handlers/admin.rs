use globed_shared::info;

use crate::{
    data::*,
    server_thread::{GameServerThread, ServerThreadMessage},
};

use super::*;

impl GameServerThread {
    gs_handler!(self, handle_admin_auth, AdminAuthPacket, packet, {
        let account_id = gs_needauth!(self);

        if !packet
            .key
            .constant_time_compare(&self.game_server.central_conf.lock().admin_key)
        {
            info!(
                "[{} ({}) @ {}] just failed to login to the admin panel with password: {}",
                self.account_data.lock().name,
                account_id,
                self.peer,
                packet.key
            );
            return Ok(());
        }

        self.is_admin.store(true, Ordering::Relaxed);

        info!(
            "[{} ({}) @ {}] just logged into the admin panel",
            self.account_data.lock().name,
            account_id,
            self.peer
        );

        self.send_packet_static(&AdminAuthSuccessPacket).await
    });

    gs_handler!(self, handle_admin_send_notice, AdminSendNoticePacket, packet, {
        let account_id = gs_needauth!(self);

        if !self.is_admin.load(Ordering::Relaxed) {
            return Ok(());
        }

        let notice_packet = ServerNoticePacket {
            message: packet.message.try_to_str(),
        };

        // i am not proud of this code
        match packet.notice_type {
            AdminSendNoticeType::Everyone => {
                let threads = self
                    .game_server
                    .threads
                    .lock()
                    .values()
                    .filter(|thr| thr.authenticated())
                    .cloned()
                    .collect::<Vec<_>>();

                info!(
                    "[{} ({}) @ {}] is sending the message to all {} people on the server: \"{}\"",
                    self.account_data.lock().name,
                    account_id,
                    self.peer,
                    threads.len(),
                    notice_packet.message,
                );

                for thread in threads {
                    thread.send_packet_dynamic(&notice_packet).await?;
                }
            }

            AdminSendNoticeType::Person => {
                let thread = self.game_server.get_user_by_name_or_id(&packet.player);

                info!(
                    "[{} ({}) @ {}] is sending the message to {}: \"{}\"",
                    self.account_data.lock().name,
                    account_id,
                    self.peer,
                    thread.as_ref().map_or_else(
                        || "<invalid player>".to_owned(),
                        |thr| thr.account_data.lock().name.try_to_str().to_owned()
                    ),
                    notice_packet.message,
                );

                if let Some(thread) = thread {
                    thread.send_packet_dynamic(&notice_packet).await?;
                }
            }

            AdminSendNoticeType::RoomOrLevel => {
                if packet.room_id != 0 && !self.game_server.state.room_manager.is_valid_room(packet.room_id) {
                    return Ok(());
                }

                let player_ids = self.game_server.state.room_manager.with_any(packet.room_id, |pm| {
                    let mut player_ids = Vec::with_capacity(128);
                    if packet.level_id == 0 {
                        pm.for_each_player(
                            |player, _, player_ids| {
                                player_ids.push(player.account_id);
                                true
                            },
                            &mut player_ids,
                        );
                    } else {
                        pm.for_each_player_on_level(
                            packet.level_id,
                            |player, _, player_ids| {
                                player_ids.push(player.account_id);
                                true
                            },
                            &mut player_ids,
                        );
                    }

                    player_ids
                });

                let threads = self
                    .game_server
                    .threads
                    .lock()
                    .values()
                    .filter(|thr| player_ids.contains(&thr.account_id.load(Ordering::Relaxed)))
                    .cloned()
                    .collect::<Vec<_>>();

                info!(
                    "[{} ({}) @ {}] is sending the message to {} people: \"{}\"",
                    self.account_data.lock().name,
                    account_id,
                    self.peer,
                    threads.len(),
                    notice_packet.message,
                );

                for thread in threads {
                    thread.send_packet_dynamic(&notice_packet).await?;
                }
            }
        }

        Ok(())
    });

    gs_handler!(self, handle_admin_disconnect, AdminDisconnectPacket, packet, {
        let _ = gs_needauth!(self);

        if !self.is_admin.load(Ordering::Relaxed) {
            return Ok(());
        }

        if let Some(thread) = self.game_server.get_user_by_name_or_id(&packet.player) {
            thread.push_new_message(ServerThreadMessage::TerminationNotice(packet.message));
        }

        Ok(())
    });
}
