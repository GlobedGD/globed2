use std::sync::atomic::Ordering;

use super::*;

impl ClientThread {
    gs_handler!(self, handle_ping, PingPacket, packet, {
        self.send_packet_static(&PingResponsePacket {
            id: packet.id,
            player_count: self.game_server.state.get_player_count(),
        })
        .await
    });

    gs_handler!(self, handle_keepalive, KeepalivePacket, _packet, {
        let _ = gs_needauth!(self);

        self.send_packet_static(&KeepaliveResponsePacket {
            player_count: self.game_server.state.get_player_count(),
        })
        .await
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
