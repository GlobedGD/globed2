use std::sync::atomic::Ordering;

use globed_shared::{anyhow::anyhow, crypto_box::ChaChaBox, logger::*, PROTOCOL_VERSION};

use crate::server_thread::{GameServerThread, PacketHandlingError};

use super::*;
use crate::data::*;

impl GameServerThread {
    gs_handler!(self, handle_ping, PingPacket, packet, {
        self.send_packet_static(&PingResponsePacket {
            id: packet.id,
            player_count: self.game_server.state.player_count.load(Ordering::Relaxed),
        })
        .await
    });

    gs_handler!(self, handle_crypto_handshake, CryptoHandshakeStartPacket, packet, {
        if packet.protocol != PROTOCOL_VERSION {
            // TODO: bring back ProtocolMismatchPacket in the future
            // self.terminate();
            // self.send_packet_static(&ProtocolMismatchPacket {
            //     protocol: PROTOCOL_VERSION,
            // })
            // .await?;
            // return Ok(());

            let message = format!("Outdated version. The server is running protocol v{} while you are running protocol v{}. Please update the mod in order to connect.", PROTOCOL_VERSION, packet.protocol);
            gs_disconnect!(self, &message);
        }

        {
            // as ServerThread is now tied to the SocketAddrV4 and not account id like in globed v0
            // erroring here is not a concern, even if the user's game crashes without a disconnect packet,
            // they would have a new randomized port when they restart and this would never fail.
            if self.crypto_box.get().is_some() {
                self.disconnect("attempting to perform a second handshake in one session")
                    .await?;
                return Err(PacketHandlingError::WrongCryptoBoxState);
            }

            self.crypto_box
                .get_or_init(|| ChaChaBox::new(&packet.key.0, &self.game_server.secret_key));
        }

        self.send_packet_static(&CryptoHandshakeResponsePacket {
            key: self.game_server.public_key.clone().into(),
        })
        .await
    });

    gs_handler!(self, handle_keepalive, KeepalivePacket, _packet, {
        let _ = gs_needauth!(self);

        self.send_packet_static(&KeepaliveResponsePacket {
            player_count: self.game_server.state.player_count.load(Ordering::Relaxed),
        })
        .await
    });

    gs_handler!(self, handle_login, LoginPacket, packet, {
        // disconnect if server is under maintenance
        if self.game_server.bridge.central_conf.lock().maintenance {
            gs_disconnect!(
                self,
                "The server is currently under maintenance, please try connecting again later."
            );
        }

        if packet.fragmentation_limit < 1300 {
            gs_disconnect!(
                self,
                &format!(
                    "The client fragmentation limit is too low ({} bytes) to be accepted",
                    packet.fragmentation_limit
                )
            );
        }

        self.fragmentation_limit.store(packet.fragmentation_limit, Ordering::Relaxed);

        if packet.account_id <= 0 || packet.user_id <= 0 {
            self.terminate();
            self.send_packet_dynamic(&LoginFailedPacket { message: "Invalid account ID was sent. Please note that you must be signed into a Geometry Dash account before connecting." }).await?;
            return Ok(());
        }

        // skip authentication if standalone
        let standalone = self.game_server.standalone;
        let player_name = if standalone {
            packet.name
        } else {
            // lets verify the given token
            let result = {
                self.game_server.bridge.token_issuer.lock().validate(
                    packet.account_id,
                    packet.user_id,
                    packet.token.to_str().unwrap(),
                )
            };

            match result {
                Ok(x) => FastString::from_str(&x),
                Err(err) => {
                    self.terminate();

                    let mut message = FastString::<80>::from_str("authentication failed: ");
                    message.extend(err.error_message()); // no need to use extend_safe as the messages are pretty short

                    self.send_packet_dynamic(&LoginFailedPacket {
                        // safety: we have created the string ourselves, we know for certain it is valid UTF-8.
                        message: unsafe { message.to_str_unchecked() },
                    })
                    .await?;
                    return Ok(());
                }
            }
        };

        // check if the user is already logged in, kick the other instance
        self.game_server.check_already_logged_in(packet.account_id).await?;

        // fetch data from the central
        if !standalone {
            let user_entry = match self.game_server.bridge.get_user_data(&packet.account_id.to_string()).await {
                Ok(user) if user.is_banned => {
                    self.terminate();
                    self.send_packet_dynamic(&LoginFailedPacket {
                        message: &format!(
                            "Banned from the server: {}",
                            user.violation_reason
                                .as_ref()
                                .map_or_else(|| "no reason given", |str| str.try_to_str())
                        ),
                    })
                    .await?;

                    return Ok(());
                }
                Ok(user) if self.game_server.bridge.is_whitelist() && !user.is_whitelisted => {
                    self.terminate();
                    self.send_packet_dynamic(&LoginFailedPacket {
                        message: "This server has whitelist enabled and your account has not been allowed.",
                    })
                    .await?;

                    return Ok(());
                }
                Ok(user) => user,
                Err(err) => {
                    self.terminate();

                    let mut message = FastString::<256>::from_str("failed to fetch user data: ");
                    message.extend(&err.to_string());

                    self.send_packet_dynamic(&LoginFailedPacket {
                        // safety: same as above
                        message: unsafe { message.to_str_unchecked() },
                    })
                    .await?;
                    return Ok(());
                }
            };

            *self.user_entry.lock() = user_entry;
        }

        self.account_id.store(packet.account_id, Ordering::Relaxed);
        self.claim_secret_key.store(packet.secret_key, Ordering::Relaxed);
        self.game_server.state.player_count.fetch_add(1u32, Ordering::Relaxed); // increment player count

        info!(
            "Login successful from {player_name} (account ID: {}, address: {})",
            packet.account_id, self.tcp_peer
        );

        let special_user_data = {
            let mut account_data = self.account_data.lock();
            account_data.account_id = packet.account_id;
            account_data.user_id = packet.user_id;
            account_data.icons.clone_from(&packet.icons);
            account_data.name = player_name;

            let name_color = self.user_entry.lock().name_color.clone();
            let sud = if let Some(name_color) = name_color {
                Some(SpecialUserData {
                    name_color: name_color
                        .to_str()
                        .map_err(|e| anyhow!("failed to parse color: {e}"))?
                        .parse()?,
                })
            } else {
                None
            };

            account_data.special_user_data = sud.clone();
            sud
        };

        // add them to the global room
        self.game_server
            .state
            .room_manager
            .get_global()
            .create_player(packet.account_id);

        let tps = self.game_server.bridge.central_conf.lock().tps;
        self.send_packet_static(&LoggedInPacket { tps, special_user_data }).await?;

        Ok(())
    });

    gs_handler_sync!(self, handle_disconnect, DisconnectPacket, _packet, {
        self.terminate();
        Ok(())
    });

    gs_handler!(self, handle_keepalive_tcp, KeepaliveTCPPacket, _packet, {
        let _ = gs_needauth!(self);

        self.send_packet_static(&KeepaliveTCPResponsePacket).await
    });

    gs_handler!(self, handle_connection_test, ConnectionTestPacket, packet, {
        self.send_packet_dynamic(&ConnectionTestResponsePacket {
            uid: packet.uid,
            data: packet.data,
        })
        .await
    });
}
