mod connection;
mod game;
mod general;

use crate::make_uninit;
pub use game::{MAX_VOICE_PACKET_SIZE, MAX_VOICE_THROUGHPUT};

/// packet handler for a specific packet type
macro_rules! gs_handler {
    ($self:ident,$name:ident,$pktty:ty,$pkt:ident,$code:expr) => {
        pub(crate) async fn $name(&$self, buf: &mut esp::ByteReader<'_>) -> crate::server_thread::Result<()> {
            let $pkt = <$pktty>::decode_from_reader(buf)?;

            if <$pktty>::PACKET_ID != PlayerDataPacket::PACKET_ID && <$pktty>::PACKET_ID != KeepalivePacket::PACKET_ID {
                $self.print_packet::<$pktty>(false, None);
            }

            $code
        }
    };
}

/// packet handler except not async
macro_rules! gs_handler_sync {
    ($self:ident,$name:ident,$pktty:ty,$pkt:ident,$code:expr) => {
        pub(crate) fn $name(&$self, buf: &mut esp::ByteReader<'_>) -> crate::server_thread::Result<()> {
            let $pkt = <$pktty>::decode_from_reader(buf)?;
            $self.print_packet::<$pktty>(false, Some("sync"));
            $code
        }
    };
}

/// call disconnect and return from the function
macro_rules! gs_disconnect {
    ($self:ident, $msg:expr) => {
        $self.disconnect($msg).await?;
        return Ok(());
    };
}

#[allow(unused_macros)]
/// send a `ServerNoticePacket` to the client with the given message
macro_rules! gs_notice {
    ($self:expr, $msg:expr) => {
        $self.send_packet_fast(&ServerNoticePacket { message: $msg }).await?;
    };
}

/// if the client isn't authenticated, invoke `gs_disconnect!` and exit the handler
macro_rules! gs_needauth {
    ($self:ident) => {{
        let account_id = $self.account_id.load(Ordering::Relaxed);

        if account_id == 0 {
            gs_disconnect!($self, FastString::from_str("unauthorized, please try connecting again"));
        }

        account_id
    }};
}

pub const MAX_ALLOCA_SIZE: usize = 100_000;

macro_rules! gs_alloca_check_size {
    ($size:expr) => {
        let size = $size;
        if size > crate::server_thread::handlers::MAX_ALLOCA_SIZE {
            // panic in debug, return an error in release mode
            if cfg!(debug_assertions) {
                panic!(
                    "attempted to allocate {} bytes on the stack - this is above the limit of {} and indicates a logic error.",
                    size,
                    crate::server_thread::handlers::MAX_ALLOCA_SIZE
                );
            }

            let err = crate::server_thread::error::PacketHandlingError::DangerousAllocation(size);
            return Err(err);
        }
    };
}

/// hype af (variable size stack allocation)
macro_rules! gs_with_alloca {
    ($size:expr, $data:ident, $code:expr) => {
        alloca::with_alloca($size, move |data| {
            // safety: 'data' will have garbage data but it's a &[u8] so that doesn't really matter.
            // we pass the responsibility to the caller to make sure they don't read any uninitialized data.
            let $data = unsafe { make_uninit!(data) };

            $code
        })
    };
}

/// i love over optimizing things!
/// when invoked as `gs_inline_encode!(self, size, buf, {code})`, uses alloca to make space on the stack,
/// and lets you encode a packet. afterwards automatically tries a non-blocking send and on failure falls back to a Vec<u8> and an async send.
macro_rules! gs_inline_encode {
    ($self:ident, $size:expr, $data:ident, $code:expr) => {
        gs_inline_encode!($self, $size, $data, _rawdata, $code)
    };

    ($self:ident, $size:expr, $data:ident, $rawdata:ident, $code:expr) => {
        gs_alloca_check_size!($size);

        let retval: Result<Option<Vec<u8>>> = {
            gs_with_alloca!($size, $rawdata, {
                let mut $data = FastByteBuffer::new($rawdata);

                $code // user code

                let data = $data.as_bytes();
                match $self.send_buffer_immediate(data) {
                    // if we cant send without blocking, accept our defeat and clone the data to a vec
                    Err(PacketHandlingError::SocketWouldBlock) => Ok(Some(data.to_vec())),
                    // if another error occured, propagate it up
                    Err(e) => Err(e),
                    // if all good, do nothing
                    Ok(()) => Ok(None),
                }
            })
        };

        if let Some(data) = retval? {
            $self.send_buffer(&data).await?;
        }
    }
}

pub(crate) use gs_alloca_check_size;
pub(crate) use gs_disconnect;
pub(crate) use gs_handler;
pub(crate) use gs_handler_sync;
pub(crate) use gs_inline_encode;
pub(crate) use gs_needauth;
pub(crate) use gs_with_alloca;

#[allow(unused_imports)]
pub(crate) use gs_notice;
