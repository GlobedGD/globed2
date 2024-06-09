use crate::server::GameServer;
use std::mem::MaybeUninit;

/// packet handler for a specific packet type
macro_rules! gs_handler {
    ($self:ident,$name:ident,$pktty:ty,$pkt:ident,$code:expr) => {
        pub(crate) async fn $name(&$self, buf: &mut esp::ByteReader<'_>) -> crate::client::Result<()> {
            let $pkt = <$pktty>::decode_from_reader(buf)?;

            #[cfg(debug_assertions)]
            if <$pktty>::PACKET_ID != KeepalivePacket::PACKET_ID {
                unsafe { $self.socket.get_mut() }.print_packet::<$pktty>(false, None);
            }

            $code
        }
    };
}

/// packet handler except not async
macro_rules! gs_handler_sync {
    ($self:ident,$name:ident,$pktty:ty,$pkt:ident,$code:expr) => {
        pub(crate) fn $name(&$self, buf: &mut esp::ByteReader<'_>) -> crate::client::Result<()> {
            let $pkt = <$pktty>::decode_from_reader(buf)?;

            #[cfg(debug_assertions)]
            unsafe { $self.socket.get_mut() }.print_packet::<$pktty>(false, Some("sync"));

            $code
        }
    };
}

/// call disconnect and return from the function
macro_rules! gs_disconnect {
    ($self:ident, $msg:expr) => {
        $self.kick($msg).await?;
        return Ok(());
    };
}

#[allow(unused_macros)]
/// send a `ServerNoticePacket` to the client with the given message
macro_rules! gs_notice {
    ($self:expr, $msg:expr) => {
        $self.send_packet_fast_dynamic(&ServerNoticePacket { message: $msg }).await?;
    };
}

/// if the client isn't authenticated, invoke `gs_disconnect!` and exit the handler
macro_rules! gs_needauth {
    ($self:ident) => {{
        let account_id = $self.account_id.load(Ordering::Relaxed);

        if account_id == 0 {
            gs_disconnect!($self, "unauthorized, please try connecting again");
        }

        account_id
    }};
}

/* data stuff */

/// Creates a new array `[u8; N]` on the stack with uninitiailized memory
#[macro_export]
macro_rules! new_uninit {
    ($size:expr) => {{
        let mut arr: [std::mem::MaybeUninit<u8>; $size] = std::mem::MaybeUninit::uninit().assume_init();
        make_uninit!(&mut arr)
    }};
}

/// Converts a `&[MaybeUninit<u8>]` into `&mut [u8]`
#[macro_export]
macro_rules! make_uninit {
    ($slc:expr) => {{
        let arr_ptr = $slc.as_mut_ptr().cast::<u8>();
        std::slice::from_raw_parts_mut(arr_ptr, std::mem::size_of_val($slc))
    }};
}

// give a larger limit on unix based machines as they often have an 8mb stack
#[cfg(windows)]
pub const MAX_ALLOCA_SIZE: usize = 2usize.pow(16);
#[cfg(not(windows))]
pub const MAX_ALLOCA_SIZE: usize = 2usize.pow(17);

macro_rules! gs_alloca_check_size {
    ($size:expr) => {
        let size = $size;
        if size > crate::client::macros::MAX_ALLOCA_SIZE {
            // panic in debug, return an error in release mode
            if cfg!(debug_assertions) {
                panic!(
                    "attempted to allocate {} bytes on the stack - this is above the limit of {} and indicates a likely logic error.",
                    size,
                    crate::client::macros::MAX_ALLOCA_SIZE
                );
            }

            let err = crate::client::error::PacketHandlingError::DangerousAllocation(size);
            return Err(err);
        }
    };
}

/// hype af (variable size stack allocation)
macro_rules! gs_with_alloca {
    ($size:expr, $data:ident, $code:expr) => {
        alloca::with_alloca($size, |data| {
            // safety: 'data' will have garbage data but it's a &[u8] so that doesn't really matter.
            // we pass the responsibility to the caller to make sure they don't read any uninitialized data.
            let $data = unsafe { crate::client::macros::make_uninit!(data) };

            $code
        })
    };
}

pub fn with_heap_vec<R>(game_server: &GameServer, size: usize, f: impl FnOnce(&mut [u8]) -> R) -> R {
    let mut buf = game_server.large_packet_buffer.lock();

    if size > buf.len() {
        drop(buf);
        let mut vec = Vec::<MaybeUninit<u8>>::with_capacity(size);

        // safety: uninit data
        unsafe {
            vec.set_len(size);
            let ptr = vec.as_mut_ptr();
            let slice = std::slice::from_raw_parts_mut(ptr, size);
            let data = make_uninit!(slice);

            f(data)
        }
    } else {
        f(&mut buf)
    }
}

/// like `gs_with_alloca!` but falls back to heap on large sizes to prevent stack overflows
macro_rules! gs_with_alloca_guarded {
    ($gs:expr, $size:expr, $data:ident, $code:expr) => {
        if $size > crate::client::macros::MAX_ALLOCA_SIZE {
            crate::client::macros::with_heap_vec($gs, $size, |$data| $code)
        } else {
            gs_with_alloca!($size, $data, $code)
        }
    };
}

/// i love over optimizing things!
/// when invoked as `gs_inline_encode!(self, size, buf, {code})`, uses alloca to make space on the stack,
/// and lets you encode a packet. afterwards automatically tries a non-blocking send and on failure falls back to a Vec<u8> and an async send.
macro_rules! gs_inline_encode {
    ($self:ident, $size:expr, $data:ident, $tcp:expr, $code:expr) => {
        gs_inline_encode!($self, $size, $data, $tcp, _rawdata, $code)
    };

    ($self:ident, $size:expr, $data:ident, $tcp:expr, $rawdata:ident, $code:expr) => {
        let retval: Result<Option<Vec<u8>>> = {
            gs_with_alloca_guarded!($self.game_server, $size, $rawdata, {
                let mut $data = FastByteBuffer::new($rawdata);

                if $tcp {
                    // reserve space for packet length
                    $data.write_u32(0);
                }

                $code // user code

                if $tcp {
                    // write the packet length
                    let packet_len = $data.len() - size_of_types!(u32);
                    let pos = $data.get_pos();
                    $data.set_pos(0);
                    $data.write_u32(packet_len as u32);
                    $data.set_pos(pos);
                }

                let data = $data.as_bytes();
                let res = if $tcp {
                    $self.send_buffer_tcp_immediate(data)
                } else {
                    $self.send_buffer_udp_immediate(data)
                };

                match res {
                    // if we cant send without blocking, accept our defeat and clone the data to a vec
                    Err(PacketHandlingError::SocketWouldBlock) => Ok(Some(data.to_vec())),
                    // if another error occured, propagate it up
                    Err(e) => Err(e),
                    // if all good, do nothing
                    Ok(written) => {
                        if written == data.len() {
                            Ok(None)
                        } else {
                            // send leftover data
                            Ok(Some(data[written..data.len()].to_vec()))
                        }
                    },
                }
            })
        };

        if let Some(data) = retval? {
            if $tcp {
                $self.send_buffer_tcp(&data).await?;
            } else {
                $self.send_buffer_udp(&data).await?;
            }
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
pub(crate) use gs_with_alloca_guarded;
pub(crate) use make_uninit;
#[allow(unused)]
pub(crate) use new_uninit;

#[allow(unused_imports)]
pub(crate) use gs_notice;
