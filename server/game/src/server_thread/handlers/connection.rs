use std::sync::atomic::Ordering;

use crypto_box::ChaChaBox;
use globed_shared::PROTOCOL_VERSION;

use crate::server_thread::{GameServerThread, PacketHandlingError};
use log::debug;

use super::*;
use crate::data::*;

impl GameServerThread {
    gs_handler!(self, handle_ping, PingPacket, packet, {
        self.send_packet_fast(&PingResponsePacket {
            id: packet.id,
            player_count: self.game_server.state.player_count.load(Ordering::Relaxed),
        })
        .await
    });

    gs_handler!(self, handle_crypto_handshake, CryptoHandshakeStartPacket, packet, {
        match packet.protocol {
            p if p > PROTOCOL_VERSION => {
                gs_disconnect!(
                    self,
                    format!(
                        "Outdated server! You are running protocol v{p} while the server is still on v{PROTOCOL_VERSION}.",
                    )
                );
            }
            p if p < PROTOCOL_VERSION => {
                gs_disconnect!(
                    self,
                    format!(
                        "Outdated client! Please update the mod in order to connect to the server. Client protocol version: v{p}, server: v{PROTOCOL_VERSION}",
                    )
                );
            }
            _ => {}
        }

        {
            let mut cbox = self.crypto_box.lock();

            // as ServerThread is now tied to the SocketAddrV4 and not account id like in globed v0
            // erroring here is not a concern, even if the user's game crashes without a disconnect packet,
            // they would have a new randomized port when they restart and this would never fail.
            if cbox.is_some() {
                return Err(PacketHandlingError::WrongCryptoBoxState);
            }

            let new_box = ChaChaBox::new(&packet.key.pubkey, &self.game_server.secret_key);
            *cbox = Some(new_box);
        }

        self.send_packet_fast(&CryptoHandshakeResponsePacket {
            key: CryptoPublicKey {
                pubkey: self.game_server.secret_key.public_key().clone(),
            },
        })
        .await
    });

    gs_handler!(self, handle_keepalive, KeepalivePacket, _packet, {
        gs_needauth!(self);

        self.send_packet_fast(&KeepaliveResponsePacket {
            player_count: self.game_server.state.player_count.load(Ordering::Relaxed),
        })
        .await
    });

    gs_handler!(self, handle_login, LoginPacket, packet, {
        if self.game_server.standalone {
            debug!("Bypassing login for {}", packet.account_id);
            self.game_server.check_already_logged_in(packet.account_id)?;
            self.authenticated.store(true, Ordering::Relaxed);
            self.account_id.store(packet.account_id, Ordering::Relaxed);
            self.game_server.state.player_count.fetch_add(1u32, Ordering::Relaxed);
            {
                let mut account_data = self.account_data.lock();
                account_data.account_id = packet.account_id;
                account_data.name = format!("Player{}", packet.account_id);
            }
            self.send_packet_fast(&LoggedInPacket {}).await?;
            return Ok(());
        }

        // lets verify the given token

        let url = format!("{}gs/verify", self.game_server.config.central_url);

        let response = self
            .game_server
            .config
            .http_client
            .post(url)
            .query(&[
                ("account_id", packet.account_id.to_string()),
                ("token", packet.token.clone()),
                ("pw", self.game_server.config.central_pw.clone()),
            ])
            .send()
            .await?
            .error_for_status()?
            .text()
            .await?;

        if !response.starts_with("status_ok:") {
            self.terminate();
            self.send_packet(&LoginFailedPacket {
                message: format!("authentication failed: {response}"),
            })
            .await?;

            return Ok(());
        }

        let player_name = response
            .split_once(':')
            .ok_or(PacketHandlingError::UnexpectedCentralResponse)?
            .1;

        self.game_server.check_already_logged_in(packet.account_id)?;
        self.authenticated.store(true, Ordering::Relaxed);
        self.account_id.store(packet.account_id, Ordering::Relaxed);
        self.game_server.state.player_count.fetch_add(1u32, Ordering::Relaxed); // increment player count

        {
            let mut account_data = self.account_data.lock();
            account_data.account_id = packet.account_id;
            account_data.name = player_name.to_string();

            let special_user_data = self
                .game_server
                .central_conf
                .lock()
                .special_users
                .get(&packet.account_id)
                .cloned();

            if let Some(sud) = special_user_data {
                account_data.special_user_data = Some(sud.try_into()?);
            }
        }

        debug!("Login successful from {player_name} ({})", packet.account_id);

        self.send_packet_fast(&LoggedInPacket {}).await?;

        Ok(())
    });

    gs_handler_sync!(self, handle_disconnect, DisconnectPacket, _packet, {
        self.terminate();
        Ok(())
    });
}
