mod connection;
mod game;

pub use game::MAX_VOICE_PACKET_SIZE;

/// packet handler for a specific packet type
macro_rules! gs_handler {
    ($self:ident,$name:ident,$pktty:ty,$pkt:ident,$code:expr) => {
        pub(crate) async fn $name(&$self, buf: &mut bytebuffer::ByteReader<'_>) -> crate::server_thread::Result<()> {
            let $pkt = <$pktty>::decode_from_reader(buf)?;
            #[cfg(debug_assertions)]
            log::debug!(
                "[{} @ {}] Handling packet {}",
                $self.account_id.load(Ordering::Relaxed),
                $self.peer,
                <$pktty>::NAME
            );
            $code
        }
    };
}

/// packet handler except not async
macro_rules! gs_handler_sync {
    ($self:ident,$name:ident,$pktty:ty,$pkt:ident,$code:expr) => {
        pub(crate) fn $name(&$self, buf: &mut bytebuffer::ByteReader<'_>) -> crate::server_thread::Result<()> {
            let $pkt = <$pktty>::decode_from_reader(buf)?;
            #[cfg(debug_assertions)]
            log::debug!(
                "[{} @ {}] Handling packet {}",
                $self.account_id.load(Ordering::Relaxed),
                $self.peer,
                <$pktty>::NAME
            );
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
    ($self:ident) => {
        if !$self.authenticated.load(Ordering::Relaxed) {
            gs_disconnect!($self, FastString::from_str("unauthorized, please try connecting again"));
        }
    };
}

pub(crate) use gs_disconnect;
pub(crate) use gs_handler;
pub(crate) use gs_handler_sync;
pub(crate) use gs_needauth;

#[allow(unused_imports)]
pub(crate) use gs_notice;
