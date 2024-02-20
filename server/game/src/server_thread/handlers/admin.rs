use globed_shared::info;

use crate::{
    data::*,
    server_thread::{GameServerThread, ServerThreadMessage},
};

use super::*;

// Role documentation:
// 0 - regular user
// 1 - can change is_muted, disconnect people
// 2 - can do all above, change is_banned, is_whitelisted, violation_reason, violation_expiry, name_color, send notices
// 100 - can do all above, change user_role, admin_password, send notices to everyone, disconnect everyone

const ROLE_HELPER: i32 = 1;
const ROLE_MOD: i32 = 2;
const ROLE_ADMIN: i32 = 100;

const ADMIN_REQUIRED_MESSAGE: &str = "unable to perform this action, Admin role is required.";
const MOD_REQUIRED_MESSAGE: &str = "unable to perform this action, at least Moderator role is required.";

macro_rules! admin_error {
    ($self:expr, $msg:expr) => {
        $self.send_packet_dynamic(&AdminErrorPacket { message: $msg }).await?;
        return Ok(());
    };
}

impl GameServerThread {
    gs_handler!(self, handle_admin_auth, AdminAuthPacket, packet, {
        let account_id = gs_needauth!(self);

        // test for the global password first
        if packet
            .key
            .constant_time_compare(&self.game_server.bridge.central_conf.lock().admin_key)
        {
            info!(
                "[{} ({}) @ {}] just logged into the admin panel (with global password)",
                self.account_data.lock().name,
                account_id,
                self.tcp_peer
            );

            self.is_authorized_admin.store(true, Ordering::Relaxed);
            // give temporary admin perms
            self.user_entry.lock().user_role = ROLE_ADMIN;
            self.send_packet_static(&AdminAuthSuccessPacket).await?;
            return Ok(());
        }

        // test for the per-user password
        let admin_password = self.user_entry.lock().admin_password.clone();

        if admin_password.as_ref().is_some_and(|pwd| !pwd.is_empty()) {
            let password = admin_password.unwrap();

            if packet.key.constant_time_compare(&password) {
                info!(
                    "[{} ({}) @ {}] just logged into the admin panel",
                    self.account_data.lock().name,
                    account_id,
                    self.tcp_peer
                );

                self.is_authorized_admin.store(true, Ordering::Relaxed);
                self.send_packet_static(&AdminAuthSuccessPacket).await?;
                return Ok(());
            }
        }

        info!(
            "[{} ({}) @ {}] just failed to login to the admin panel with password: {}",
            self.account_data.lock().name,
            account_id,
            self.tcp_peer,
            packet.key
        );
        return Ok(());
    });

    gs_handler!(self, handle_admin_send_notice, AdminSendNoticePacket, packet, {
        let account_id = gs_needauth!(self);

        if !self.is_authorized_admin.load(Ordering::Relaxed) {
            return Ok(());
        }

        // require at least mod
        let role = self.user_entry.lock().user_role;
        if role < ROLE_MOD {
            admin_error!(self, MOD_REQUIRED_MESSAGE);
        }

        let notice_packet = ServerNoticePacket { message: packet.message };

        // i am not proud of this code
        match packet.notice_type {
            AdminSendNoticeType::Everyone => {
                if role < ROLE_ADMIN {
                    admin_error!(self, ADMIN_REQUIRED_MESSAGE);
                }

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
                    self.tcp_peer,
                    threads.len(),
                    notice_packet.message,
                );

                self.send_packet_dynamic(&AdminSuccessMessagePacket {
                    message: &format!("Sent to {} people", threads.len()),
                })
                .await?;

                for thread in threads {
                    thread
                        .push_new_message(ServerThreadMessage::BroadcastNotice(notice_packet.clone()))
                        .await;
                }
            }

            AdminSendNoticeType::Person => {
                let thread = self.game_server.find_user(&packet.player);

                let player_name = thread.as_ref().map_or_else(
                    || "<invalid player>".to_owned(),
                    |thr| thr.account_data.lock().name.try_to_str().to_owned(),
                );

                info!(
                    "[{} ({}) @ {}] is sending the message to {}: \"{}\"",
                    self.account_data.lock().name,
                    account_id,
                    self.tcp_peer,
                    player_name,
                    notice_packet.message,
                );

                if let Some(thread) = thread {
                    thread
                        .push_new_message(ServerThreadMessage::BroadcastNotice(notice_packet.clone()))
                        .await;

                    self.send_packet_dynamic(&AdminSuccessMessagePacket {
                        message: &format!("Sent notice to {}", thread.account_data.lock().name),
                    })
                    .await?;
                } else {
                    admin_error!(self, "failed to find the user");
                }
            }

            AdminSendNoticeType::RoomOrLevel => {
                if packet.room_id != 0 && !self.game_server.state.room_manager.is_valid_room(packet.room_id) {
                    admin_error!(self, "unable to send notice, invalid room ID");
                }

                if packet.room_id == 0 && role < ROLE_ADMIN {
                    admin_error!(self, ADMIN_REQUIRED_MESSAGE);
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
                    self.tcp_peer,
                    threads.len(),
                    notice_packet.message,
                );

                self.send_packet_dynamic(&AdminSuccessMessagePacket {
                    message: &format!("Sent to {} people", threads.len()),
                })
                .await?;

                for thread in threads {
                    thread
                        .push_new_message(ServerThreadMessage::BroadcastNotice(notice_packet.clone()))
                        .await;
                }
            }
        }

        Ok(())
    });

    gs_handler!(self, handle_admin_disconnect, AdminDisconnectPacket, packet, {
        let _ = gs_needauth!(self);

        if !self.is_authorized_admin.load(Ordering::Relaxed) {
            return Ok(());
        }

        // require at least helper
        let role = self.user_entry.lock().user_role;
        if role < ROLE_HELPER {
            return Ok(());
        }

        // to kick everyone, require admin
        if &*packet.player == "@everyone" && role >= ROLE_ADMIN {
            let threads: Vec<_> = self.game_server.threads.lock().values().cloned().collect();
            for thread in threads {
                thread
                    .push_new_message(ServerThreadMessage::TerminationNotice(packet.message.clone()))
                    .await;
            }

            return Ok(());
        }

        if let Some(thread) = self.game_server.find_user(&packet.player) {
            thread
                .push_new_message(ServerThreadMessage::TerminationNotice(packet.message))
                .await;

            self.send_packet_dynamic(&AdminSuccessMessagePacket {
                message: &format!("Successfully kicked {}", thread.account_data.lock().name),
            })
            .await
        } else {
            admin_error!(self, "failed to find the user");
        }
    });

    gs_handler!(self, handle_admin_get_user_state, AdminGetUserStatePacket, packet, {
        let _ = gs_needauth!(self);

        if !self.is_authorized_admin.load(Ordering::Relaxed) {
            return Ok(());
        }

        // require at least helper
        let role = self.user_entry.lock().user_role;
        if role < ROLE_HELPER {
            return Ok(());
        }

        let user = self.game_server.find_user(&packet.player);
        if let Some(user) = user {
            let entry = user.user_entry.lock().clone();
            self.send_packet_static(&AdminUserDataPacket { entry }).await?;
        } else {
            admin_error!(self, "failed to find the user");
        }

        Ok(())
    });

    gs_handler!(self, handle_admin_update_user, AdminUpdateUserPacket, packet, {
        let _ = gs_needauth!(self);

        if !self.is_authorized_admin.load(Ordering::Relaxed) {
            return Ok(());
        }

        // require at least helper
        let role = self.user_entry.lock().user_role;
        if role < ROLE_HELPER {
            return Ok(());
        }

        // evaluate the changes
        let thread = self.game_server.find_user(&packet.player);
        if thread.is_none() {
            admin_error!(self, "failed to find the user to update");
        }

        let thread = thread.unwrap();
        let user_entry = thread.user_entry.lock().clone();

        // first check for actions that require admin rights
        if role < ROLE_ADMIN
            && (packet.user_entry.user_role != user_entry.user_role
                || packet.user_entry.admin_password != user_entry.admin_password)
        {
            admin_error!(self, ADMIN_REQUIRED_MESSAGE);
        }

        // check for actions that require mod rights
        if role < ROLE_MOD
            && (packet.user_entry.is_banned != user_entry.is_banned
                || packet.user_entry.is_whitelisted != user_entry.is_whitelisted
                || packet.user_entry.violation_reason != user_entry.violation_reason
                || packet.user_entry.violation_expiry != user_entry.violation_expiry
                || packet.user_entry.name_color != user_entry.name_color)
        {
            admin_error!(self, MOD_REQUIRED_MESSAGE);
        }

        let result = self
            .game_server
            .update_user(&thread, move |user| {
                user.clone_from(&packet.user_entry);
                true
            })
            .await;

        match result {
            Ok(()) => {
                let player_name = thread.account_data.lock().name.clone();

                info!(
                    "[{} @ {}] just updated the profile of {}",
                    self.account_data.lock().name,
                    self.tcp_peer,
                    player_name
                );

                self.send_packet_dynamic(&AdminSuccessMessagePacket {
                    message: "Successfully updated the user",
                })
                .await
            }
            Err(err) => {
                admin_error!(self, &err.to_string());
            }
        }
    });
}
