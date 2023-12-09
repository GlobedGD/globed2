use std::sync::atomic::Ordering;

use crate::{
    data::packets::PacketHeader,
    server_thread::{GameServerThread, PacketHandlingError, Result},
};

use super::*;
use crate::data::*;

impl GameServerThread {
    gs_handler!(self, handle_sync_icons, SyncIconsPacket, packet, {
        gs_needauth!(self);

        self.account_data.lock().icons.clone_from(&packet.icons);
        Ok(())
    });

    gs_handler!(self, handle_request_player_list, RequestPlayerListPacket, _packet, {
        gs_needauth!(self);

        let player_count = self.game_server.state.player_count.load(Ordering::Relaxed);
        let encoded_size = size_of_types!(PlayerPreviewAccountData) * player_count as usize;

        gs_inline_encode!(self, encoded_size, buf, {
            buf.write_packet_header::<PlayerListPacket>();
            buf.write_u32(player_count);

            let written = self.game_server.for_every_player_preview(
                move |preview, count, buf| {
                    // we do additional length check because player count may have increased since then
                    if count < player_count as usize {
                        buf.write_value(preview);
                        true
                    } else {
                        false
                    }
                },
                &mut buf,
            ) as u32;

            // if the player count has instead decreased, we now lied and the client will fail decoding. re-encode the actual count.
            if written != player_count {
                buf.set_pos(PacketHeader::SIZE);
                buf.write_u32(written);
            }
        });

        Ok(())
    });
}
