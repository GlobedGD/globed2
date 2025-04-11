use globed_shared::webhook::WebhookMessage;

use super::*;

impl ClientThread {
    gs_handler!(self, handle_sync_icons, SyncIconsPacket, packet, {
        let _ = gs_needauth!(self);

        self.account_data.lock().icons.clone_from(&packet.icons);
        Ok(())
    });

    gs_handler!(self, handle_request_global_list, RequestGlobalPlayerListPacket, _packet, {
        let _ = gs_needauth!(self);

        self.send_packet_dynamic(&GlobalPlayerListPacket {
            players: self.game_server.get_player_previews_in_room(0, self.can_moderate()),
        })
        .await
    });

    gs_handler!(self, handle_request_level_list, RequestLevelListPacket, _packet, {
        let _ = gs_needauth!(self);

        let levels = {
            let room = self.room.lock();

            let manager = room.manager.read();

            let mut vec = Vec::with_capacity(manager.get_level_count());

            manager.for_each_level(|level_id, level| {
                if !level.unlisted && !is_editorcollab_level(level_id) {
                    vec.push(GlobedLevel {
                        level_id,
                        player_count: level.players.len() as u16,
                    });
                }
            });

            vec
        };

        self.send_packet_dynamic(&LevelListPacket { levels }).await
    });

    gs_handler!(self, handle_request_player_count, RequestPlayerCountPacket, packet, {
        let _ = gs_needauth!(self);

        let levels = {
            let room = self.room.lock();
            let mut levels = Vec::with_capacity(0);

            for &level_id in &*packet.level_ids {
                levels.push((level_id, room.get_player_count_on_level(level_id).unwrap_or(0) as u16));
            }

            levels
        };

        self.send_packet_dynamic(&LevelPlayerCountPacket { levels }).await
    });

    gs_handler!(self, handle_set_player_status, UpdatePlayerStatusPacket, packet, {
        let account_id = gs_needauth!(self);

        let mut p = self.privacy_settings.lock();
        p.clone_from(&packet.flags);

        // only mods can hide roles
        let is_mod = self.is_authorized_user.load(Ordering::Relaxed);

        if !is_mod {
            p.set_hide_roles(false);
        }

        if let Some(player) = self.room.lock().manager.write().get_player_data_mut(account_id) {
            player.is_invisible = packet.flags.get_hide_in_game();
        }

        Ok(())
    });

    gs_handler!(self, handle_link_code_request, LinkCodeRequestPacket, _packet, {
        let _ = gs_needauth!(self);

        let link_code = self.link_code.load(Ordering::Relaxed);

        self.send_packet_static(&LinkCodeResponsePacket { link_code }).await
    });

    gs_handler!(self, handle_notice_reply, NoticeReplyPacket, packet, {
        let _ = gs_needauth!(self);

        if packet.message.len() > MAX_NOTICE_SIZE {
            gs_disconnect!(self, "notice too long");
        }

        let reply = {
            let mut replies = self.pending_notice_replies.lock();

            replies
                .iter()
                .position(|reply| reply.reply_id == packet.reply_id)
                .map(|pos| replies.remove(pos))
        };

        if let Some(mut reply) = reply {
            reply.user_reply = packet.message;

            let moderator = self.game_server.get_user_by_id(reply.moderator_id);

            if self.game_server.bridge.has_admin_webhook() {
                let self_name = self.account_data.lock().name.try_to_string();
                let mod_name = if let Some(moderator) = moderator.as_ref() {
                    moderator.account_data.lock().name.try_to_string()
                } else {
                    format!("<unknown> ({})", reply.moderator_id)
                };

                if let Err(err) = self
                    .game_server
                    .bridge
                    .send_admin_webhook_message(WebhookMessage::NoticeReply(
                        self_name,
                        mod_name,
                        reply.user_reply.try_to_string(),
                        reply.reply_id,
                    ))
                    .await
                {
                    warn!("webhook error during notice to person: {err}");
                }
            }

            // find the recipient moderator
            if let Some(moderator) = moderator {
                moderator.push_new_message(ServerThreadMessage::BroadcastNoticeReply(reply)).await;
            } else {
                gs_notice!(self, "Error: Failed to send notice reply, the moderator went offline");
            }
        } else {
            gs_notice!(self, "Error: Failed to send notice reply, maybe it expired?");
        }

        Ok(())
    });

    gs_handler!(self, handle_motd_request, RequestMotdPacket, packet, {
        let _ = gs_needauth!(self);

        let res = {
            let conf = self.game_server.bridge.central_conf.lock();
            if (conf.motd.is_empty() || conf.motd_hash.eq_ignore_ascii_case(packet.motd_hash.try_to_str())) && !packet.expect_response {
                None
            } else {
                Some((conf.motd.clone(), conf.motd_hash.clone()))
            }
        };

        if let Some((motd, motd_hash)) = res {
            self.send_packet_dynamic(&MotdResponsePacket { motd, motd_hash }).await?;
        }

        Ok(())
    });
}
