mod connection;
mod game;

macro_rules! gs_handler {
    ($self:ident,$name:ident,$pktty:ty,$pkt:ident,$code:expr) => {
        pub(crate) async fn $name(&$self, buf: &mut bytebuffer::ByteReader<'_>) -> crate::server_thread::Result<()> {
            let $pkt = <$pktty>::decode_from_reader(buf)?;
            #[cfg(debug_assertions)]
            log::debug!("Handling packet {}", <$pktty>::NAME);
            $code
        }
    };
}

macro_rules! gs_handler_sync {
    ($self:ident,$name:ident,$pktty:ty,$pkt:ident,$code:expr) => {
        pub(crate) fn $name(&$self, buf: &mut bytebuffer::ByteReader<'_>) -> crate::server_thread::Result<()> {
            let $pkt = <$pktty>::decode_from_reader(buf)?;
            #[cfg(debug_assertions)]
            log::debug!("Handling packet {}", <$pktty>::NAME);
            $code
        }
    };
}

macro_rules! gs_disconnect {
    ($self:ident, $msg:expr) => {
        $self.terminate();
        $self
            .send_packet_fast(&ServerDisconnectPacket { message: $msg })
            .await?;
        return Ok(());
    };
}

#[allow(unused_macros)]
macro_rules! gs_notice {
    ($self:expr, $msg:expr) => {
        $self.send_packet(&ServerNoticePacket { message: $msg }).await?;
    };
}

macro_rules! gs_needauth {
    ($self:ident) => {
        if !$self.authenticated.load(Ordering::Relaxed) {
            gs_disconnect!($self, "unauthorized, please try connecting again".to_string());
        }
    };
}

pub(crate) use gs_disconnect;
pub(crate) use gs_handler;
pub(crate) use gs_handler_sync;
pub(crate) use gs_needauth;

#[allow(unused_imports)]
pub(crate) use gs_notice;
