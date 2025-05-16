use std::borrow::Cow;
use std::time::UNIX_EPOCH;

use globed_shared::{data::*, info, warn};

use crate::{bridge::AdminUserAction, data::v14::LevelId, managers::ComputedRole, webhook::WebhookMessage};

use super::*;

macro_rules! admin_error {
    ($self:expr, $msg:literal) => {
        $self
            .send_packet_dynamic(&AdminErrorPacket {
                message: Cow::Borrowed($msg),
            })
            .await?;
        return Ok(());
    };

    ($self:expr, $msg:expr) => {
        $self.send_packet_dynamic(&AdminErrorPacket { message: Cow::Owned($msg) }).await?;
        return Ok(());
    };
}

#[derive(Clone, Copy)]
enum AdminPerm {
    Any,
    Notice,
    NoticeToEveryone,
    Kick,
    KickEveryone,
    Mute,
    Ban,
    EditRoles,
    // EditFeaturedLevels,
    Admin,
}

impl ClientThread {
    // check if the user is logged in as admin, and if they have the given permission
    fn _has_perm(&self, perm: AdminPerm) -> bool {
        if !self.is_authorized_user.load(Ordering::Relaxed) {
            return false;
        }

        let role = self.user_role.lock();

        if role.admin {
            return true;
        }

        match perm {
            AdminPerm::Any => role.can_moderate(),
            AdminPerm::Notice => role.notices,
            AdminPerm::NoticeToEveryone => role.notices_to_everyone,
            AdminPerm::Ban => role.ban,
            AdminPerm::Mute => role.mute,
            AdminPerm::Kick => role.kick,
            AdminPerm::KickEveryone => role.kick_everyone,
            AdminPerm::EditRoles => role.edit_role,
            // AdminPerm::EditFeaturedLevels => role.edit_featured_levels,
            AdminPerm::Admin => false,
        }
    }

    fn _update_user_role(&self, from: &ComputedRole) {
        self.user_role.lock().clone_from(from);
    }

    gs_handler!(self, handle_admin_auth, AdminAuthPacket, packet, {
        let account_id = gs_needauth!(self);

        // make sure we exist in the db!
        self._verify_user_exists(account_id).await?;

        // test for the global password first
        if packet.key.constant_time_compare(&self.game_server.bridge.central_conf.lock().admin_key) {
            info!(
                "[{} ({}) @ {}] just logged into the admin panel (with global password)",
                self.account_data.lock().name,
                account_id,
                self.get_tcp_peer()
            );

            self.is_authorized_user.store(true, Ordering::Relaxed);
            // give super admin perms
            let role = self.game_server.state.role_manager.get_superadmin();
            self._update_user_role(&role);

            self.send_packet_dynamic(&AdminAuthSuccessPacket { role }).await?;

            return Ok(());
        }

        // test for the per-user password
        let pass_result = self.user_entry.lock().verify_password(&packet.key);

        match pass_result {
            Ok(true) => {
                info!(
                    "[{} ({}) @ {}] just logged into the admin panel",
                    self.account_data.lock().name,
                    account_id,
                    self.get_tcp_peer()
                );

                self.is_authorized_user.store(true, Ordering::Relaxed);

                let role = self.game_server.state.role_manager.compute(&self.user_entry.lock().user_roles);
                self._update_user_role(&role);

                self.send_packet_dynamic(&AdminAuthSuccessPacket { role }).await?;

                return Ok(());
            }
            Ok(false) => {}
            Err(e) => {
                warn!("argon2 password validation error: {e}");
            }
        }

        info!(
            "[{} ({}) @ {}] just failed to login to the admin panel with password: {}",
            self.account_data.lock().name,
            account_id,
            self.get_tcp_peer(),
            packet.key
        );

        // this is silly tbh
        // if self.game_server.bridge.has_webhook() {
        //     let name = self.account_data.lock().name.try_to_string();
        //
        //     if let Err(err) = self.game_server.bridge.send_webhook_message(WebhookMessage::AuthFail(name)).await {
        //         warn!("webhook error: {err}");
        //     }
        // }

        self.send_packet_static(&AdminAuthFailedPacket).await
    });

    gs_handler!(self, handle_admin_send_notice, AdminSendNoticePacket, packet, {
        let account_id = gs_needauth!(self);

        if !self._has_perm(AdminPerm::Notice) {
            return Ok(());
        }

        if packet.message.len() > MAX_NOTICE_SIZE {
            admin_error!(self, "message is too long");
        }

        if packet.message.is_empty() {
            return Ok(());
        }

        let notice_packet = ServerNoticePacket {
            message: packet.message,
            reply_id: 0,
        };

        match packet.notice_type {
            AdminSendNoticeType::Everyone => self._send_notice_to_everyone(account_id, packet.just_estimate, notice_packet).await,

            AdminSendNoticeType::Person => {
                self._send_notice_to_person(account_id, &packet.player, packet.can_reply, packet.just_estimate, notice_packet)
                    .await
            }

            AdminSendNoticeType::RoomOrLevel => {
                self._send_notice_to_room_or_level(account_id, packet.room_id, packet.level_id, packet.just_estimate, notice_packet)
                    .await
            }
        }
    });

    async fn _send_notice_to_everyone(&self, account_id: i32, estimate: bool, packet: ServerNoticePacket) -> Result<()> {
        if !self._has_perm(AdminPerm::NoticeToEveryone) {
            admin_error!(self, "no permission");
        }

        if estimate {
            let est = self.game_server.clients.lock().values().filter(|thr| thr.authenticated()).count();
            return self._send_estimate(est).await;
        }

        let threads = self
            .game_server
            .clients
            .lock()
            .values()
            .filter(|thr| thr.authenticated())
            .cloned()
            .collect::<Vec<_>>();

        let name = self.account_data.lock().name.try_to_string();

        info!(
            "[{name} ({account_id}) @ {}] sending a notice to all {} people on the server: {}",
            self.get_tcp_peer(),
            threads.len(),
            packet.message,
        );

        if self.game_server.bridge.has_admin_webhook()
            && let Err(err) = self
                .game_server
                .bridge
                .send_admin_webhook_message(WebhookMessage::NoticeToEveryone(name, threads.len(), packet.message.try_to_string()))
                .await
        {
            warn!("webhook error during notice to everyone: {err}");
        }

        self.send_packet_dynamic(&AdminSuccessMessagePacket {
            message: Cow::Owned(format!("Sent to {} people", threads.len())),
        })
        .await?;

        for thread in threads {
            thread.push_new_message(ServerThreadMessage::BroadcastNotice(packet.clone())).await;
        }

        Ok(())
    }

    async fn _send_notice_to_person(
        &self,
        account_id: i32,
        target: &str,
        can_reply: bool,
        estimate: bool,
        mut packet: ServerNoticePacket,
    ) -> Result<()> {
        let thread = self.game_server.find_user(target);

        if estimate {
            return self._send_estimate(thread.is_some() as usize).await;
        }

        let player_name = thread.as_ref().map_or_else(
            || "<invalid player>".to_owned(),
            |thr| thr.account_data.lock().name.try_to_str().to_owned(),
        );

        let self_name = self.account_data.lock().name.try_to_string();
        let notice_msg = packet.message.try_to_string();

        info!(
            "[{self_name} ({account_id}) @ {}] sending a notice to {player_name}: {notice_msg}",
            self.get_tcp_peer()
        );

        // set the reply id
        if let Some(thread) = thread.as_ref()
            && can_reply
        {
            packet.reply_id = thread.create_notice_reply(account_id, packet.message.clone());
        }

        if self.game_server.bridge.has_admin_webhook()
            && let Err(err) = self
                .game_server
                .bridge
                .send_admin_webhook_message(WebhookMessage::NoticeToPerson(self_name, player_name, notice_msg, packet.reply_id))
                .await
        {
            warn!("webhook error during notice to person: {err}");
        }

        if let Some(thread) = thread {
            thread.push_new_message(ServerThreadMessage::BroadcastNotice(packet)).await;

            self.send_packet_dynamic(&AdminSuccessMessagePacket {
                message: Cow::Owned(format!("Sent notice to {}", thread.account_data.lock().name)),
            })
            .await?;
        } else {
            admin_error!(self, "failed to find the user");
        }

        Ok(())
    }

    async fn _send_notice_to_room_or_level(
        &self,
        account_id: i32,
        room_id: u32,
        level_id: LevelId,
        estimate: bool,
        packet: ServerNoticePacket,
    ) -> Result<()> {
        if room_id != 0 && !self.game_server.state.room_manager.is_valid_room(room_id) {
            admin_error!(self, "unable to send notice, invalid room ID");
        }

        // if this is a global room, also require the notice to everyone perm
        if room_id == 0 && !self._has_perm(AdminPerm::NoticeToEveryone) {
            admin_error!(self, "no permission");
        }

        let player_ids = self.game_server.state.room_manager.with_any(room_id, |pm| {
            let mut player_ids = Vec::with_capacity(128);
            if level_id == 0 {
                pm.manager.read().for_each_player(|player| {
                    player_ids.push(player.account_id);
                });
            } else {
                pm.manager.read().for_each_player_on_level(level_id, |player| {
                    player_ids.push(player.account_id);
                });
            }

            player_ids
        });

        if estimate {
            return self._send_estimate(player_ids.len()).await;
        }

        let threads = self
            .game_server
            .clients
            .lock()
            .values()
            .filter(|thr| player_ids.contains(&thr.account_id.load(Ordering::Relaxed)))
            .cloned()
            .collect::<Vec<_>>();

        let self_name = self.account_data.lock().name.try_to_string();
        let notice_msg = packet.message.try_to_string();

        info!(
            "[{self_name} ({account_id}) @ {}] sending a notice to {} people: {notice_msg}",
            self.get_tcp_peer(),
            threads.len()
        );

        if self.game_server.bridge.has_admin_webhook()
            && let Err(err) = self
                .game_server
                .bridge
                .send_admin_webhook_message(WebhookMessage::NoticeToSelection(self_name, threads.len(), notice_msg))
                .await
        {
            warn!("webhook error during notice to selection: {err}");
        }

        self.send_packet_dynamic(&AdminSuccessMessagePacket {
            message: Cow::Owned(format!("Sent to {} people", threads.len())),
        })
        .await?;

        for thread in threads {
            thread.push_new_message(ServerThreadMessage::BroadcastNotice(packet.clone())).await;
        }

        Ok(())
    }

    async fn _send_estimate(&self, count: usize) -> Result<()> {
        self.send_packet_static(&AdminNoticeRecipientCountPacket { count: count as u32 }).await
    }

    gs_handler!(self, handle_admin_disconnect, AdminDisconnectPacket, packet, {
        let _ = gs_needauth!(self);

        if !self._has_perm(AdminPerm::Kick) {
            return Ok(());
        }

        // to kick everyone, require admin
        if &*packet.player == "@everyone" && self._has_perm(AdminPerm::KickEveryone) {
            let threads: Vec<_> = self.game_server.clients.lock().values().cloned().collect();
            for thread in threads {
                thread
                    .push_new_message(ServerThreadMessage::TerminationNotice(packet.message.clone()))
                    .await;
            }

            let self_name = self.account_data.lock().name.try_to_string();

            if self.game_server.bridge.has_admin_webhook()
                && let Err(err) = self
                    .game_server
                    .bridge
                    .send_admin_webhook_message(WebhookMessage::KickEveryone(self_name, packet.message.try_to_string()))
                    .await
            {
                warn!("webhook error during kick everyone: {err}");
            }

            return Ok(());
        }

        if let Some(thread) = self.game_server.find_user(&packet.player) {
            let reason_string = packet.message.try_to_string();

            thread.push_new_message(ServerThreadMessage::TerminationNotice(packet.message)).await;

            if self.game_server.bridge.has_admin_webhook() {
                let own_name = self.account_data.lock().name.try_to_string();
                let target_name = thread.account_data.lock().name.try_to_string();

                if let Err(err) = self
                    .game_server
                    .bridge
                    .send_admin_webhook_message(WebhookMessage::KickPerson(
                        own_name,
                        target_name,
                        thread.account_id.load(Ordering::Relaxed),
                        reason_string,
                    ))
                    .await
                {
                    warn!("webhook error during kick person: {err}");
                }
            }

            self.send_packet_dynamic(&AdminSuccessMessagePacket {
                message: Cow::Owned(format!("Successfully kicked {}", thread.account_data.lock().name)),
            })
            .await
        } else {
            admin_error!(self, "failed to find the user");
        }
    });

    gs_handler!(self, handle_admin_get_user_state, AdminGetUserStatePacket, packet, {
        let _ = gs_needauth!(self);

        if !self._has_perm(AdminPerm::Any) {
            return Ok(());
        }

        let user = self.game_server.find_user(&packet.player);
        let account_data = user.as_ref().map(|x| x.account_data.lock().make_room_preview(0, true));

        // on a standalone server, if the user is not online we are kinda yeah
        let user_entry = if self.game_server.standalone
            && let Some(user) = user
        {
            user.user_entry.lock().clone().to_user_entry(None, None)
        } else {
            // request data via the bridge
            match self.game_server.bridge.get_user_data(&packet.player).await {
                Ok(x) => x,
                Err(err) => {
                    warn!("error fetching data from the bridge: {err}");
                    admin_error!(self, err.to_string());
                }
            }
        };

        let packet = AdminUserDataPacket {
            entry: user_entry,
            account_data,
        };

        self.send_packet_dynamic(&packet).await
    });

    gs_handler!(self, handle_admin_send_featured_level, AdminSendFeaturedLevelPacket, packet, {
        let account_id = gs_needauth!(self);
        let self_name = self.account_data.lock().name.try_to_string();

        if !self._has_perm(AdminPerm::Any) {
            return Ok(());
        }

        if let Err(err) = self
            .game_server
            .bridge
            .send_rate_suggestion_webhook_message(WebhookMessage::FeaturedLevelSend(
                account_id,
                self_name,
                packet.level_name.to_string(),
                packet.level_id,
                packet.level_author.to_string(),
                packet.difficulty,
                packet.rate_tier,
                packet.notes.map(|x| x.to_string()),
            ))
            .await
        {
            warn!("webhook error during send featured: {err}");
        }

        return Ok(());
    });

    // Handle user edit packets

    gs_handler!(self, handle_admin_update_username, AdminUpdateUsernamePacket, packet, {
        let _ = gs_needauth!(self);

        self._handle_admin_action(
            packet.account_id,
            AdminPerm::Any,
            &AdminUserAction::UpdateUsername(AdminUpdateUsernameAction {
                account_id: packet.account_id,
                username: packet.username,
            }),
        )
        .await?;

        self._send_admin_success(packet.account_id).await
    });

    gs_handler!(self, handle_admin_set_name_color, AdminSetNameColorPacket, packet, {
        let account_id = gs_needauth!(self);

        self._verify_user_exists(packet.account_id).await?;

        self._handle_admin_action(
            packet.account_id,
            AdminPerm::EditRoles,
            &AdminUserAction::SetNameColor(AdminSetNameColorAction {
                account_id: packet.account_id,
                color: packet.color.to_fast_string(),
                issued_by: account_id,
            }),
        )
        .await?;

        self._send_admin_success(packet.account_id).await
    });

    gs_handler!(self, handle_admin_set_user_roles, AdminSetUserRolesPacket, packet, {
        let account_id = gs_needauth!(self);

        if !self.game_server.state.role_manager.all_valid(&packet.roles) {
            admin_error!(self, "attempting to assign an invalid role");
        }

        self._verify_user_exists(packet.account_id).await?;

        let thread = self.game_server.get_user_by_id(packet.account_id);
        let editing_self = account_id == packet.account_id;

        let my_priority = self.game_server.state.role_manager.compute_priority(&self.user_entry.lock().user_roles);
        let their_new_priority = self.game_server.state.role_manager.compute_priority(&packet.roles);

        let their_priority = if editing_self {
            my_priority
        } else if let Some(user) = &thread {
            user.user_role.lock().priority
        } else {
            match self.game_server.bridge.get_user_data(&packet.account_id).await {
                Ok(x) => self.game_server.state.role_manager.compute_priority(&x.user_roles),
                Err(err) => {
                    warn!("error fetching data from the bridge: {err}");
                    admin_error!(self, err.to_string());
                }
            }
        };

        // disallow editing overall if we <= them, and additionally disallow assigning roles >= us
        if !self._has_perm(AdminPerm::Admin) {
            if my_priority <= their_priority && !editing_self {
                admin_error!(self, "cannot edit roles of a user with higher roles than you");
            } else if their_new_priority >= my_priority {
                admin_error!(self, "cannot set user's roles to be higher than yours");
            }
        }

        self._handle_admin_action(
            packet.account_id,
            AdminPerm::EditRoles,
            &AdminUserAction::SetUserRoles(AdminSetUserRolesAction {
                account_id: packet.account_id,
                roles: packet.roles,
                issued_by: account_id,
            }),
        )
        .await?;

        // if the user is online on the server, update live
        if let Some(user) = self.game_server.get_user_by_id(packet.account_id) {
            let pkt = ServerThreadMessage::BroadcastRoleChange(RolesUpdatedPacket {
                special_user_data: user.account_data.lock().special_user_data.clone(),
            });

            user.push_new_message(pkt).await;
        }

        self._send_admin_success(packet.account_id).await
    });

    gs_handler!(self, handle_admin_punish_user, AdminPunishUserPacket, packet, {
        let account_id = gs_needauth!(self);

        // verify the punishment is correct
        if packet.expires_at != 0 && UNIX_EPOCH.elapsed().unwrap().as_secs() > packet.expires_at {
            admin_error!(self, "invalid expiration date");
        }

        self._verify_user_exists(packet.account_id).await?;

        let thread = self.game_server.get_user_by_id(packet.account_id);
        let editing_self = account_id == packet.account_id;

        let my_priority = self.game_server.state.role_manager.compute_priority(&self.user_entry.lock().user_roles);

        let their_priority = if editing_self {
            my_priority
        } else if let Some(user) = &thread {
            user.user_role.lock().priority
        } else {
            // oh well we gotta make a bridge req to get their prio
            match self.game_server.bridge.get_user_data(&packet.account_id).await {
                Ok(x) => self.game_server.state.role_manager.compute_priority(&x.user_roles),
                Err(err) => {
                    warn!("error fetching data from the bridge: {err}");
                    admin_error!(self, err.to_string());
                }
            }
        };

        // cannot ban role above you or at your level
        if packet.is_ban && my_priority <= their_priority && !self._has_perm(AdminPerm::Admin) {
            admin_error!(self, "cannot ban user above or at your permission level");
        }

        let _ = self
            ._handle_admin_action(
                packet.account_id,
                if packet.is_ban { AdminPerm::Ban } else { AdminPerm::Mute },
                &AdminUserAction::PunishUser(AdminPunishUserAction {
                    account_id: packet.account_id,
                    is_ban: packet.is_ban,
                    reason: packet.reason.clone(),
                    issued_by: account_id,
                    expires_at: packet.expires_at,
                }),
            )
            .await?;

        // if the user is online on the server, update live
        if let Some(user) = thread {
            if packet.is_ban {
                user.push_new_message(ServerThreadMessage::BroadcastBan(ServerBannedPacket {
                    message: FastString::new(&packet.reason),
                    expires_at: packet.expires_at,
                }))
                .await;
            } else {
                user.push_new_message(ServerThreadMessage::BroadcastMute(ServerMutedPacket {
                    reason: packet.reason,
                    expires_at: packet.expires_at,
                }))
                .await;
            }
        }

        self._send_admin_success(packet.account_id).await
    });

    gs_handler!(self, handle_admin_remove_punishment, AdminRemovePunishmentPacket, packet, {
        let account_id = gs_needauth!(self);

        self._verify_user_exists(packet.account_id).await?;

        self._handle_admin_action(
            packet.account_id,
            if packet.is_ban { AdminPerm::Ban } else { AdminPerm::Mute },
            &AdminUserAction::RemovePunishment(AdminRemovePunishmentAction {
                account_id: packet.account_id,
                is_ban: packet.is_ban,
                issued_by: account_id,
            }),
        )
        .await?;

        self._send_admin_success(packet.account_id).await
    });

    gs_handler!(self, handle_admin_whitelist, AdminWhitelistPacket, packet, {
        let account_id = gs_needauth!(self);

        self._verify_user_exists(packet.account_id).await?;

        self._handle_admin_action(
            packet.account_id,
            AdminPerm::Ban,
            &AdminUserAction::Whitelist(AdminWhitelistAction {
                account_id: packet.account_id,
                state: packet.state,
                issued_by: account_id,
            }),
        )
        .await?;

        self._send_admin_success(packet.account_id).await
    });

    gs_handler!(self, handle_admin_set_admin_password, AdminSetAdminPasswordPacket, packet, {
        let _ = gs_needauth!(self);

        self._verify_user_exists(packet.account_id).await?;

        self._handle_admin_action(
            packet.account_id,
            AdminPerm::Admin,
            &AdminUserAction::SetAdminPassword(AdminSetAdminPasswordAction {
                account_id: packet.account_id,
                new_password: packet.new_password,
            }),
        )
        .await?;

        self._send_admin_success(packet.account_id).await
    });

    gs_handler!(self, handle_admin_edit_punishment, AdminEditPunishmentPacket, packet, {
        let account_id = gs_needauth!(self);

        // verify the punishment is correct
        if packet.expires_at != 0 && UNIX_EPOCH.elapsed().unwrap().as_secs() > packet.expires_at {
            admin_error!(self, "invalid expiration date");
        }

        self._verify_user_exists(packet.account_id).await?;

        self._handle_admin_action(
            packet.account_id,
            if packet.is_ban { AdminPerm::Ban } else { AdminPerm::Mute },
            &AdminUserAction::EditPunishment(AdminEditPunishmentAction {
                account_id: packet.account_id,
                is_ban: packet.is_ban,
                reason: packet.reason,
                expires_at: packet.expires_at,
                issued_by: account_id,
            }),
        )
        .await?;

        self._send_admin_success(packet.account_id).await
    });

    // Return
    async fn _handle_admin_action(
        &self,
        user_account_id: i32,
        perm: AdminPerm,
        action: &AdminUserAction,
    ) -> Result<(Option<UserPunishment>, Option<UserPunishment>)> {
        if !self._has_perm(perm) {
            return Err(PacketHandlingError::NoPermission);
        }

        if self.game_server.standalone {
            self.send_packet_dynamic(&AdminErrorPacket {
                message: Cow::Borrowed("This cannot be done on a standalone server"),
            })
            .await?;

            return Err(PacketHandlingError::Standalone);
        }

        match self.game_server.bridge.send_admin_user_action(action).await {
            Ok((x, ban, mute)) => {
                if self.game_server.bridge.has_admin_webhook() {
                    let own_name = self.account_data.lock().name.clone();
                    let account_id = self.account_id.load(Ordering::Relaxed);

                    let user_name: Cow<'_, str> = if let Some(u) = &x.user_name {
                        Cow::Borrowed(u)
                    } else if let Some(user) = self.game_server.get_user_by_id(user_account_id) {
                        Cow::Owned(user.account_data.lock().name.try_to_string())
                    } else {
                        Cow::Owned("Unknown".to_owned())
                    };

                    if let Err(e) = self
                        .game_server
                        .bridge
                        .send_webhook_message_for_action(action, &x, account_id, own_name.try_to_string(), user_name, ban.as_ref(), mute.as_ref())
                        .await
                    {
                        error!("Failed to submit webhook message for admin action: {e}");
                    }
                }

                if let Some(user) = self.game_server.get_user_by_id(x.account_id) {
                    let sud = SpecialUserData::from_roles(&x.user_roles, &self.game_server.state.role_manager);

                    user.account_data.lock().special_user_data = sud;
                    let mut user_role = user.user_role.lock();

                    // dont replace if they have super admin
                    if user_role.priority != self.game_server.state.role_manager.get_superadmin().priority {
                        *user_role = self.game_server.state.role_manager.compute(&x.user_roles);
                    }
                    *user.user_entry.lock() = x;
                }

                Ok((ban, mute))
            }

            Err(e) => {
                self.send_packet_dynamic(&AdminErrorPacket {
                    message: Cow::Owned(e.to_string()),
                })
                .await?;

                Err(PacketHandlingError::BridgeError(e))
            }
        }
    }

    async fn _send_admin_success(&self, account_id: i32) -> Result<()> {
        let user_entry = match self.game_server.bridge.get_user_data(&account_id).await {
            Ok(x) => x,
            Err(err) => return Err(PacketHandlingError::BridgeError(err)),
        };

        self.send_packet_dynamic(&AdminSuccessfulUpdatePacket { user_entry }).await
    }

    async fn _send_admin_success_msg(&self, msg: impl AsRef<str>) -> Result<()> {
        self.send_packet_dynamic(&AdminSuccessMessagePacket {
            message: Cow::Borrowed(msg.as_ref()),
        })
        .await
    }

    async fn _verify_user_exists(&self, account_id: i32) -> Result<UserEntry> {
        if self.game_server.standalone {
            return Ok(UserEntry::new(account_id));
        }

        match self.game_server.bridge.get_user_data(&account_id).await {
            Ok(u) if u.user_name.is_some() => return Ok(u),
            _ => {}
        };

        // see if we can add the user
        let name = if let Some(thread) = self.game_server.get_user_by_id(account_id) {
            thread.account_data.lock().name.clone()
        } else {
            Default::default()
        };

        let (ue, ban, mute) = self
            .game_server
            .bridge
            .send_admin_user_action(&AdminUserAction::UpdateUsername(AdminUpdateUsernameAction { account_id, username: name }))
            .await?;

        Ok(ue.to_user_entry(ban, mute))
    }

    gs_handler!(self, handle_admin_get_punishment_history, AdminGetPunishmentHistoryPacket, packet, {
        let _ = gs_needauth!(self);

        if !self._has_perm(AdminPerm::Any) {
            return Err(PacketHandlingError::NoPermission);
        }

        match self.game_server.bridge.get_punishment_history(packet.account_id).await {
            Ok(x) => {
                // ok so basically client needs to know names of all mods (as db only stores account ids)
                let mut ids = Vec::<i32>::new();
                for entry in &x {
                    if let Some(id) = entry.issued_by {
                        ids.push(id);
                    }
                }

                let mod_name_data = match self.game_server.bridge.get_many_names(&ids).await {
                    Ok(x) => x,
                    Err(err) => {
                        warn!("error fetching data from the bridge: {err}");
                        Vec::new()
                    }
                };

                self.send_packet_dynamic(&AdminPunishmentHistoryPacket { entries: x, mod_name_data })
                    .await?;
                Ok(())
            }

            Err(e) => {
                self.send_packet_dynamic(&AdminErrorPacket {
                    message: Cow::Owned(e.to_string()),
                })
                .await?;

                Err(PacketHandlingError::BridgeError(e))
            }
        }
    });
}
