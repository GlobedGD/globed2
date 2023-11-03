use crate::{
    bytebufferext::{decode_impl, encode_impl},
    data::packets::{packet, Packet},
};

packet!(PingPacket, 10000, false, {
    id: u32,
});

// encode_unimpl!(PingPacket);
// decode_unimpl!(PingPacket);

encode_impl!(PingPacket, buf, self, {
    buf.write_u32(self.id);
});

decode_impl!(PingPacket, buf, {
    Ok(PingPacket {
        id: buf.read_u32()?,
    })
});
