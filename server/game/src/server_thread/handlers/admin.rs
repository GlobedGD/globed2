use globed_shared::{info, warn};

use crate::{
    data::*,
    server_thread::{GameServerThread, ServerThreadMessage},
};

use super::*;

// Role documentation:
// 0 - regular user
// 1 - can change is_muted, violation_reason, violation_expiry, disconnect people
// 2 - can do all above, change is_banned, is_whitelisted, name_color, send notices
// 100 - can do all above, change user_role, admin_password, send notices to everyone, disconnect everyone
// 101 - can do all above, change roles of users to admin

const ROLE_USER: i32 = 0;
const ROLE_HELPER: i32 = 1;
const ROLE_MOD: i32 = 2;
const ROLE_ADMIN: i32 = 100;
const ROLE_SUPERADMIN: i32 = 101;

const ADMIN_REQUIRED_MESSAGE: &str = "unable to perform this action, not enough permissions.";
const MOD_REQUIRED_MESSAGE: &str = "unable to perform this action, not enough permissions";
const SUPERADMIN_REQUIRED_MESSAGE: &str = "unable to perform this action, not enough permissions";

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
            // give super admin perms
            self.user_entry.lock().user_role = ROLE_SUPERADMIN;
            self.send_packet_static(&AdminAuthSuccessPacket { role: ROLE_SUPERADMIN })
                .await?;
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
                let role = self.user_entry.lock().user_role;
                self.send_packet_static(&AdminAuthSuccessPacket { role }).await?;
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

        if self.game_server.bridge.has_webhook() {
            if let Err(err) = self
                .game_server
                .bridge
                .send_webhook_message(format!(
                    "{} just failed to login to the admin panel.",
                    self.account_data.lock().name
                ))
                .await
            {
                warn!("webhook error: {err}");
            }
        }

        self.send_packet_static(&AdminAuthFailedPacket).await
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

                let name = self.account_data.lock().name.clone();
                let webhook_msg = format!(
                    "{name} is sending a notice to all {} people on the server: {}",
                    threads.len(),
                    notice_packet.message,
                );

                info!("[{account_id} @ {}] {}", self.tcp_peer, webhook_msg);

                if self.game_server.bridge.has_webhook() {
                    if let Err(err) = self.game_server.bridge.send_webhook_message(webhook_msg).await {
                        warn!("webhook error: {err}");
                    }
                }

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

                let webhook_msg = format!(
                    "{} is sending a notice to {}: {}",
                    self.account_data.lock().name,
                    player_name,
                    notice_packet.message
                );

                info!("[{account_id} @ {}] {webhook_msg}", self.tcp_peer);

                if self.game_server.bridge.has_webhook() {
                    if let Err(err) = self.game_server.bridge.send_webhook_message(webhook_msg).await {
                        warn!("webhook error: {err}");
                    }
                }

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

                let webhook_msg = format!(
                    "{} is sending a notice to {} people: {}",
                    self.account_data.lock().name,
                    threads.len(),
                    notice_packet.message
                );

                info!("[{account_id} @ {}] {webhook_msg}", self.tcp_peer);

                if self.game_server.bridge.has_webhook() {
                    if let Err(err) = self.game_server.bridge.send_webhook_message(webhook_msg).await {
                        warn!("webhook error: {err}");
                    }
                }

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

            if self.game_server.bridge.has_webhook() {
                if let Err(err) = self
                    .game_server
                    .bridge
                    .send_webhook_message(format!(
                        "{} just kicked {} from the server",
                        self.account_data.lock().name,
                        thread.account_data.lock().name
                    ))
                    .await
                {
                    warn!("webhook error: {err}");
                }
            }

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
        let mut packet = if let Some(user) = user {
            let entry = user.user_entry.lock().clone();
            let account_data = user.account_data.lock().make_room_preview(0);

            AdminUserDataPacket {
                entry,
                account_data: Some(account_data),
            }
        } else {
            // on a standalone server, if the user is not online we are kinda out of luck
            if self.game_server.standalone {
                admin_error!(self, "This cannot be done on a standalone server");
            }

            // they are not on the server right now, request data via the bridge
            let user_entry = match self.game_server.bridge.get_user_data(&packet.player).await {
                Ok(x) => x,
                Err(err) => {
                    warn!("error fetching data from the bridge: {err}");
                    admin_error!(self, &err.to_string());
                }
            };

            AdminUserDataPacket {
                entry: user_entry,
                account_data: None,
            }
        };

        // only admins can see/change passwords of others
        if role < ROLE_ADMIN {
            packet.entry.admin_password = None;
        }

        self.send_packet_static(&packet).await
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

        // we cant use bridge in standalone so do nothing
        if self.game_server.standalone {
            admin_error!(self, "This cannot be done on a standalone server");
        }

        let mut new_user_entry = packet.user_entry;

        let target_account_id = new_user_entry.account_id;

        // if they are online, update them live, else get their old data from the bridge

        let thread = self.game_server.get_user_by_id(target_account_id);
        let user_entry = if let Some(thread) = thread.as_ref() {
            thread.user_entry.lock().clone()
        } else {
            match self.game_server.bridge.get_user_data(&target_account_id.to_string()).await {
                Ok(x) => x,
                Err(err) => {
                    admin_error!(self, &format!("failed to get user: {err}"));
                }
            }
        };

        // if not admin, cant update others password or role
        if role < ROLE_ADMIN {
            new_user_entry.admin_password = user_entry.admin_password.clone();
            new_user_entry.user_role = user_entry.user_role;
        }

        // check what changed
        let c_user_role = new_user_entry.user_role != user_entry.user_role;
        let c_admin_password = new_user_entry.admin_password != user_entry.admin_password;
        let c_is_banned = new_user_entry.is_banned != user_entry.is_banned;
        let c_is_muted = new_user_entry.is_muted != user_entry.is_muted;
        let c_is_whitelisted = new_user_entry.is_whitelisted != user_entry.is_whitelisted;
        let c_violation_reason = new_user_entry.violation_reason != user_entry.violation_reason;
        let c_violation_expiry = new_user_entry.violation_expiry != user_entry.violation_expiry;
        let c_name_color = new_user_entry.name_color != user_entry.name_color;
        let c_user_name = new_user_entry.user_name != user_entry.user_name;

        // first check for actions that require super admin rights
        // only superadmin can assign other admins
        if role < ROLE_SUPERADMIN && (c_user_role && new_user_entry.user_role >= ROLE_ADMIN) {
            admin_error!(self, SUPERADMIN_REQUIRED_MESSAGE);
        }

        // check for actions that require admin rights (assigning roles and changing admin passwords)
        if role < ROLE_ADMIN && (c_user_role || c_admin_password) {
            admin_error!(self, ADMIN_REQUIRED_MESSAGE);
        }

        // check for actions that require mod rights (ban, whitelist, name color)
        if role < ROLE_MOD && (c_is_banned || c_is_whitelisted || c_name_color) {
            admin_error!(self, MOD_REQUIRED_MESSAGE);
        }

        // validation
        let target_role = new_user_entry.user_role;
        if !(target_role == ROLE_USER
            || target_role == ROLE_HELPER
            || target_role == ROLE_MOD
            || target_role == ROLE_ADMIN
            || target_role == ROLE_SUPERADMIN)
        {
            admin_error!(self, "attempting to assign an invalid role");
        }

        if let Some(color) = new_user_entry.name_color.as_ref() {
            if color.parse::<Color3B>().is_err() {
                admin_error!(self, &format!("attempting to assign an invalid name color: {color}"));
            }
        }

        // user_name intentionally left unchecked.
        let _ = c_user_name;

        if !(c_user_role
            || c_admin_password
            || c_is_banned
            || c_is_muted
            || c_is_whitelisted
            || c_violation_reason
            || c_violation_expiry
            || c_name_color)
        {
            // no changes
            return self
                .send_packet_dynamic(&AdminSuccessMessagePacket { message: "No changes" })
                .await;
        }

        // if not banned and not muted, clear the violation reason and duration
        if !new_user_entry.is_banned && !new_user_entry.is_muted {
            new_user_entry.violation_expiry = None;
            new_user_entry.violation_reason = None;
        }

        let target_user_name = new_user_entry
            .user_name
            .clone()
            .unwrap_or_else(|| FastString::from_str("<unknown>"));

        // if online, update live
        let result = if let Some(thread) = thread.as_ref() {
            let is_banned = new_user_entry.is_banned;

            // update name color live
            if c_name_color {
                thread.account_data.lock().special_user_data = new_user_entry.name_color.clone().map(|x| SpecialUserData {
                    name_color: x.parse().unwrap_or_default(),
                });
            }

            let res = self
                .game_server
                .update_user(thread, |user| {
                    user.clone_from(&new_user_entry);
                    true
                })
                .await;

            // if they just got banned, disconnect them
            if is_banned && res.is_ok() {
                thread
                    .push_new_message(ServerThreadMessage::TerminationNotice(FastString::from_str(
                        "Banned from the server",
                    )))
                    .await;
            }

            res
        } else {
            // otherwise just make a manual bridge request
            self.game_server
                .bridge
                .update_user_data(&new_user_entry)
                .await
                .map_err(Into::into)
        };

        match result {
            Ok(()) => {
                info!(
                    "[{} @ {}] just updated the profile of {}",
                    self.account_data.lock().name,
                    self.tcp_peer,
                    target_account_id
                );

                if self.game_server.bridge.has_webhook() {
                    let mut webhook_msg = format!(
                        "{} just updated the user {} ({})",
                        self.account_data.lock().name,
                        target_user_name,
                        target_account_id
                    );

                    // role change
                    if c_user_role {
                        webhook_msg += &format!(
                            "\n* Role change: is now **{}**",
                            match new_user_entry.user_role {
                                ROLE_USER => "regular user",
                                ROLE_HELPER => "helper",
                                ROLE_MOD => "mod",
                                ROLE_ADMIN => "admin",
                                _ => "unknown",
                            }
                        );
                    }

                    // banned message
                    if c_is_banned {
                        webhook_msg += &if user_entry.is_banned {
                            "\n* **Banned** -> **Unbanned**".to_owned()
                        } else {
                            format!(
                                "\n* **Banned until: {}**",
                                if let Some(unix_seconds) = new_user_entry.violation_expiry {
                                    format!("<t:{unix_seconds}:f>")
                                } else {
                                    "*permanent*".to_owned()
                                }
                            )
                        };
                    }
                    // muted message
                    else if c_is_muted {
                        webhook_msg += &if user_entry.is_muted {
                            "\n* **Muted** -> **Unmuted**".to_owned()
                        } else {
                            format!(
                                "\n* **Muted until: {}**",
                                if let Some(unix_seconds) = new_user_entry.violation_expiry {
                                    format!("<t:{unix_seconds}:f>")
                                } else {
                                    "*permanent*".to_owned()
                                }
                            )
                        }
                    }

                    // whitelisted
                    if c_is_whitelisted {
                        webhook_msg += if user_entry.is_whitelisted {
                            "\n* **Whitelisted** -> **Not whitelisted**"
                        } else {
                            "\n* **Not whitelisted** -> **Whitelisted**"
                        }
                    }

                    // changing reason or duration of a ban/mute
                    if !c_is_banned && !c_is_muted && c_violation_expiry {
                        webhook_msg += &format!(
                            "\n* Ban/mute expiration changed to: {}",
                            if let Some(unix_seconds) = new_user_entry.violation_expiry {
                                format!("<t:{unix_seconds}:f>")
                            } else {
                                "*permanent*".to_owned()
                            }
                        );
                    }

                    if c_violation_reason {
                        webhook_msg += &format!(
                            "\n* Ban/mute reason: {}",
                            new_user_entry
                                .violation_reason
                                .unwrap_or_else(|| FastString::from_str("<no reason given>"))
                        );
                    }

                    // name color
                    if c_name_color {
                        webhook_msg += &format!(
                            "\n* Name color change: {}",
                            new_user_entry.name_color.clone().unwrap_or_default()
                        );
                    }

                    if let Err(err) = self.game_server.bridge.send_webhook_message(webhook_msg).await {
                        warn!("webhook error: {err}");
                    }
                }

                self.send_packet_dynamic(&AdminSuccessMessagePacket {
                    message: "Successfully updated the user",
                })
                .await
            }
            Err(err) => {
                warn!("error from bridge: {err}");
                admin_error!(self, &err.to_string());
            }
        }
    });
}
