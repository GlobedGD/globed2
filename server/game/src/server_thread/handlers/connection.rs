use std::sync::atomic::Ordering;

use globed_shared::{crypto_box::ChaChaBox, logger::*, PROTOCOL_VERSION};

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
            self.terminate();
            self.send_packet_static(&ProtocolMismatchPacket {
                protocol: PROTOCOL_VERSION,
            })
            .await?;
            return Ok(());
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
        if self.game_server.central_conf.lock().maintenance {
            gs_disconnect!(
                self,
                "The server is currently under maintenance, please try connecting again later."
            );
        }

        if packet.fragmentation_limit < 1400 {
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
            match self
                .game_server
                .token_issuer
                .validate(packet.account_id, packet.user_id, packet.token.to_str().unwrap())
            {
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

        self.game_server.check_already_logged_in(packet.account_id).await?;
        self.account_id.store(packet.account_id, Ordering::Relaxed);
        self.claim_secret_key.store(packet.secret_key, Ordering::Relaxed);
        self.game_server.state.player_count.fetch_add(1u32, Ordering::Relaxed); // increment player count

        info!(
            "Login successful from {player_name} (account ID: {}, address: {})",
            packet.account_id, self.tcp_peer
        );

        {
            let mut account_data = self.account_data.lock();
            account_data.account_id = packet.account_id;
            account_data.user_id = packet.user_id;
            account_data.icons.clone_from(&packet.icons);
            account_data.name = player_name;

            if !standalone {
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
        }

        // add them to the global room
        self.game_server
            .state
            .room_manager
            .get_global()
            .create_player(packet.account_id);

        let tps = self.game_server.central_conf.lock().tps;
        self.send_packet_static(&LoggedInPacket { tps }).await?;

        Ok(())
    });

    gs_handler_sync!(self, handle_disconnect, DisconnectPacket, _packet, {
        self.terminate();
        Ok(())
    });

    gs_handler!(self, handle_connection_test, ConnectionTestPacket, packet, {
        self.send_packet_dynamic(&ConnectionTestResponsePacket {
            uid: packet.uid,
            data: packet.data,
        })
        .await
    });
}
