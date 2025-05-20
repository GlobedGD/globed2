use std::{
    net::{SocketAddr, SocketAddrV4},
    sync::OnceLock,
    time::Duration,
};

use crate::tokio::{
    self,
    io::{AsyncReadExt, AsyncWriteExt},
    net::TcpStream,
};

use globed_shared::rand::Rng;
#[allow(unused_imports)]
use globed_shared::{
    crypto_box::{
        ChaChaBox,
        aead::{AeadCore, AeadInPlace, OsRng},
    },
    trace,
};

use super::{
    PartialTranslatableEncodable,
    error::{PacketHandlingError, Result},
    macros::*,
};
use crate::{data::*, server::GameServer};

pub struct ClientSocket {
    pub socket: TcpStream,

    pub tcp_peer: SocketAddrV4,
    pub udp_peer: Option<SocketAddrV4>,
    crypto_box: OnceLock<ChaChaBox>,
    game_server: &'static GameServer,
    protocol_version: u16,
    mtu: usize,
}

#[derive(PartialEq, Eq)]
pub enum ProtocolOverride {
    None,
    Tcp,
    Udp,
}

// do not touch those, encryption related
const NONCE_SIZE: usize = 24;
const MAC_SIZE: usize = 16;

const MAX_PACKET_SIZE: usize = 65536;
pub const INLINE_BUFFER_SIZE: usize = 164;

const MARKER_UDP_PACKET: u8 = 0xb1;
const MARKER_UDP_FRAME: u8 = 0xa7;

impl ClientSocket {
    pub fn new(socket: TcpStream, tcp_peer: SocketAddrV4, mtu: usize, game_server: &'static GameServer) -> Self {
        Self {
            socket,
            tcp_peer,
            udp_peer: None,
            crypto_box: OnceLock::new(),
            game_server,
            protocol_version: 0,
            mtu,
        }
    }

    pub async fn shutdown(&mut self) -> std::io::Result<()> {
        self.socket.shutdown().await
    }

    pub async fn poll_for_tcp_data(&mut self) -> Result<usize> {
        let mut length_buf = [0u8; 4];
        self.socket.read_exact(&mut length_buf).await?;

        Ok(u32::from_be_bytes(length_buf) as usize)
    }

    /// Receive `bytes` bytes from the TCP connection and invoke the given closure.
    #[inline]
    pub async fn recv_and_handle<F>(&mut self, bytes: usize, f: F) -> Result<()>
    where
        F: AsyncFnOnce(&mut [u8]) -> Result<()>,
    {
        if bytes > MAX_PACKET_SIZE {
            return Err(PacketHandlingError::PacketTooLong(bytes));
        }

        let mut inline_buf = [0u8; INLINE_BUFFER_SIZE];
        let mut heap_buf = Vec::<u8>::new();

        let use_inline = bytes <= INLINE_BUFFER_SIZE;

        let data: &mut [u8] = if use_inline {
            self.socket.read_exact(&mut inline_buf[..bytes]).await?;
            &mut inline_buf[..bytes]
        } else {
            let sock = &mut self.socket;

            let read_bytes = sock.take(bytes as u64).read_to_end(&mut heap_buf).await?;
            &mut heap_buf[..read_bytes]
        };

        f(data).await
    }

    pub fn init_crypto_box(&self, key: &CryptoPublicKey) -> Result<()> {
        if self.crypto_box.get().is_some() {
            return Err(PacketHandlingError::WrongCryptoBoxState);
        }

        self.crypto_box.get_or_init(|| ChaChaBox::new(&key.0, &self.game_server.secret_key));

        Ok(())
    }

    pub fn set_udp_peer(&mut self, udp_peer: SocketAddrV4) {
        self.udp_peer.replace(udp_peer);
    }

    pub fn set_mtu(&mut self, mtu: usize) {
        if mtu == 0 || mtu >= 65000 {
            self.mtu = 0; // 65k or 0 interpreted as no fragmentation
        } else {
            self.mtu = mtu;
        }
    }

    pub fn set_protocol_version(&mut self, version: u16) {
        self.protocol_version = version;
    }

    pub fn decrypt<'a>(&self, message: &'a mut [u8]) -> Result<ByteReader<'a>> {
        if message.len() < PacketHeader::SIZE + NONCE_SIZE + MAC_SIZE {
            return Err(PacketHandlingError::MalformedCiphertext);
        }

        let cbox = self.crypto_box.get();

        if cbox.is_none() {
            return Err(PacketHandlingError::WrongCryptoBoxState);
        }

        let cbox = cbox.unwrap();

        let nonce_start = PacketHeader::SIZE;
        let mac_start = nonce_start + NONCE_SIZE;
        let ciphertext_start = mac_start + MAC_SIZE;

        let mut nonce = [0u8; NONCE_SIZE];
        nonce.clone_from_slice(&message[nonce_start..mac_start]);
        let nonce = nonce.into();

        let mut mac = [0u8; MAC_SIZE];
        mac.clone_from_slice(&message[mac_start..ciphertext_start]);
        let mac = mac.into();

        cbox.decrypt_in_place_detached(&nonce, b"", &mut message[ciphertext_start..], &mac)
            .map_err(|_| PacketHandlingError::DecryptionError)?;

        Ok(ByteReader::from_bytes(&message[ciphertext_start..]))
    }

    // packet encoding and sending functions

    #[cfg(debug_assertions)]
    pub fn print_packet<P: PacketMetadata>(&self, sending: bool, ptype: Option<&str>) {
        trace!(
            "[{}] {} packet {}{}",
            self.tcp_peer,
            if sending { "Sending" } else { "Handling" },
            P::NAME,
            ptype.map_or(String::new(), |pt| format!(" ({pt})"))
        );
    }

    #[inline]
    #[cfg(not(debug_assertions))]
    pub fn print_packet<P: PacketMetadata>(&self, _sending: bool, _ptype: Option<&str>) {}

    /// fast packet sending with best-case zero heap allocation. requires the packet to implement `StaticSize`.
    /// if the packet size isn't known at compile time, derive/implement `DynamicSize` and use `send_packet_dynamic` instead.
    pub async fn send_packet_static<P: Packet + Encodable + StaticSize>(&mut self, packet: &P) -> Result<()> {
        self.send_packet_static_override(packet, ProtocolOverride::None).await
    }

    pub async fn send_packet_static_override<P: Packet + Encodable + StaticSize>(&mut self, packet: &P, proto: ProtocolOverride) -> Result<()> {
        // in theory, the size is known at compile time, so we could use a stack array here, instead of using alloca.
        // however in practice, the performance difference is negligible, so we avoid code unnecessary code repetition.
        self.send_packet_alloca(packet, P::ENCODED_SIZE, proto).await
    }

    /// version of `send_packet_static` that does not require the size to be known at compile time.
    /// you are still required to derive/implement `DynamicSize` so the size can be computed at runtime.
    pub async fn send_packet_dynamic<P: Packet + Encodable + DynamicSize>(&mut self, packet: &P) -> Result<()> {
        self.send_packet_dynamic_override(packet, ProtocolOverride::None).await
    }

    pub async fn send_packet_dynamic_override<P: Packet + Encodable + DynamicSize>(&mut self, packet: &P, proto: ProtocolOverride) -> Result<()> {
        self.send_packet_alloca(packet, packet.encoded_size(), proto).await
    }

    /// packet translation, this is not optimized for performance and is relatively slow.
    /// however, translatable packets should have a relatively low percentage compared to others, so this isn't a concern.
    pub async fn send_packet_translatable<P: Packet + Encodable + DynamicSize + PartialTranslatableEncodable>(
        &mut self,
        packet: P,
        proto: ProtocolOverride,
    ) -> Result<()> {
        // if client's protocol is same or set to ignore, send it without translating
        if self.protocol_version == CURRENT_PROTOCOL || self.protocol_version == 0xffff {
            return self.send_packet_dynamic_override(&packet, proto).await;
        }

        let mut buf = ByteBuffer::with_capacity(128);

        if let Err(e) = packet.translate_and_encode(self.protocol_version, &mut buf) {
            return Err(PacketHandlingError::TranslationError(e));
        }

        self.send_packet_alloca_with::<P, _>(buf.len(), proto, move |fb| {
            fb.write_bytes(buf.as_bytes());
        })
        .await
    }

    /// use alloca to encode the packet on the stack, and try a non-blocking send, on failure clones to a Vec and a blocking send.
    /// be very careful if using this directly, miscalculating the size may cause a runtime panic.
    #[inline]
    pub async fn send_packet_alloca<P: Packet + Encodable>(&mut self, packet: &P, packet_size: usize, proto: ProtocolOverride) -> Result<()> {
        self.send_packet_alloca_with::<P, _>(packet_size, proto, |buf| {
            buf.write_value(packet);
        })
        .await
    }

    /// low level version of other `send_packet_xxx` methods. there is no bound for `Encodable`, `StaticSize` or `DynamicSize`,
    /// but you have to provide a closure that handles packet encoding, and you must specify the appropriate packet size (upper bound).
    /// you do **not** have to encode the packet header or include its size in the `packet_size` argument, that will be done for you automatically.
    #[inline]
    pub async fn send_packet_alloca_with<P: Packet, F>(&mut self, packet_size: usize, proto: ProtocolOverride, encode_fn: F) -> Result<()>
    where
        F: FnOnce(&mut FastByteBuffer),
    {
        if cfg!(debug_assertions) && P::PACKET_ID != KeepaliveResponsePacket::PACKET_ID {
            self.print_packet::<P>(true, Some(if P::ENCRYPTED { "fast + encrypted" } else { "fast" }));
        }

        let use_tcp = proto == ProtocolOverride::Tcp || (proto == ProtocolOverride::None && P::SHOULD_USE_TCP);

        if P::ENCRYPTED {
            // gs_inline_encode! doesn't work here because the borrow checker is silly :(
            let header_start = if use_tcp { size_of_types!(u32) } else { size_of_types!(u8) };

            let nonce_start = header_start + PacketHeader::SIZE;
            let mac_start = nonce_start + NONCE_SIZE;
            let raw_data_start = mac_start + MAC_SIZE;
            let total_size = raw_data_start + packet_size;

            gs_alloca_check_size!(total_size);

            let to_send: Result<Option<Vec<u8>>> = gs_with_alloca!(total_size, data, {
                let mut buf = FastByteBuffer::new(data);

                if use_tcp {
                    // reserve space for packet length
                    buf.write_u32(0);
                } else {
                    // udp packet marker
                    buf.write_u8(MARKER_UDP_PACKET);
                }

                // write the header
                buf.write_packet_header::<P>();

                // first encode the packet
                let mut buf = FastByteBuffer::new(&mut data[raw_data_start..raw_data_start + packet_size]);
                encode_fn(&mut buf);

                // if the written size isn't equal to `packet_size`, we use buffer length instead
                let raw_data_end = raw_data_start + buf.len();

                // this unwrap is safe, as an encrypted packet can only be sent downstream after the handshake is established.
                let cbox = self.crypto_box.get().unwrap();

                // encrypt in place
                let nonce = ChaChaBox::generate_nonce(&mut OsRng);
                let tag = cbox
                    .encrypt_in_place_detached(&nonce, b"", &mut data[raw_data_start..raw_data_end])
                    .map_err(|_| PacketHandlingError::EncryptionError)?;

                // prepend the nonces
                data[nonce_start..mac_start].copy_from_slice(nonce.as_slice());

                // prepend the mac tag
                data[mac_start..raw_data_start].copy_from_slice(&tag);

                if use_tcp {
                    // write total packet length
                    let packet_len = (raw_data_end - header_start) as u32;
                    data[..size_of_types!(u32)].copy_from_slice(&packet_len.to_be_bytes());
                }

                // we try a non-blocking send if we can, otherwise fallback to a Vec<u8> and an async send
                let send_data = &data[..raw_data_end];

                let res = if use_tcp {
                    self.send_buffer_tcp_immediate(send_data)
                } else {
                    self.send_buffer_udp_immediate(send_data)
                };

                match res {
                    Err(PacketHandlingError::SocketWouldBlock) => Ok(Some(send_data.to_vec())),
                    Err(e) => Err(e),
                    Ok(written) => {
                        if written == raw_data_end {
                            Ok(None)
                        } else {
                            // send leftover data
                            Ok(Some(data[written..raw_data_end].to_vec()))
                        }
                    }
                }
            });

            if let Some(to_send) = to_send? {
                if use_tcp {
                    self.send_buffer_tcp(&to_send).await?;
                } else {
                    self.send_buffer_udp(&to_send).await?;
                }
            }
        } else {
            let prefix_sz = if use_tcp { size_of_types!(u32) } else { 1usize };
            let total_payload_size = prefix_sz + PacketHeader::SIZE + packet_size;

            // ok so now umm
            let should_fragment = !use_tcp && self.mtu != 0 && total_payload_size > self.mtu;

            // so yeah
            if should_fragment {
                let mut vec = vec![0u8; total_payload_size];
                let mut buf = FastByteBuffer::new(&mut vec[..]);

                // write packet header and packet itself
                buf.write_packet_header::<P>();
                encode_fn(&mut buf);

                let data = buf.as_bytes();
                self.send_fragmented_udp_payload(data).await?;
            } else {
                let retval: Result<Option<Vec<u8>>> = {
                    gs_with_alloca_guarded!(self.game_server, total_payload_size, raw_data, {
                        let mut buf = FastByteBuffer::new(raw_data);

                        if use_tcp {
                            // reserve space for packet length
                            buf.write_u32(0);
                        } else {
                            // write udp packet marker
                            buf.write_u8(MARKER_UDP_PACKET);
                        }

                        // write packet header and packet itself
                        buf.write_packet_header::<P>();
                        encode_fn(&mut buf);

                        if use_tcp {
                            // write the packet length
                            let packet_len = buf.len() - size_of_types!(u32);
                            let pos = buf.get_pos();
                            buf.set_pos(0);
                            buf.write_u32(packet_len as u32);
                            buf.set_pos(pos);
                        }

                        let data = buf.as_bytes();
                        let res = if use_tcp {
                            self.send_buffer_tcp_immediate(data)
                        } else {
                            self.send_buffer_udp_immediate(data)
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
                            }
                        }
                    })
                };

                if let Some(data) = retval? {
                    if use_tcp {
                        self.send_buffer_tcp(&data).await?;
                    } else {
                        self.send_buffer_udp(&data).await?;
                    }
                }
            }
        }

        if use_tcp {
            self.socket.flush().await?;
        }

        Ok(())
    }

    /// sends a buffer to our peer via the tcp socket
    async fn send_buffer_tcp(&mut self, buffer: &[u8]) -> Result<()> {
        let result = tokio::time::timeout(Duration::from_secs(5), self.socket.write_all(buffer)).await;

        match result {
            Ok(Ok(())) => Ok(()),
            Ok(Err(err)) => Err(PacketHandlingError::SocketSendFailed(err)),
            Err(_) => {
                // timed out
                Err(PacketHandlingError::SocketSendFailed(std::io::Error::new(
                    std::io::ErrorKind::TimedOut,
                    "TCP send timed out (waited for 5 seconds)",
                )))
            }
        }
    }

    /// non async version of `send_buffer_tcp`
    fn send_buffer_tcp_immediate(&mut self, buffer: &[u8]) -> Result<usize> {
        let result = self.socket.try_write(buffer);

        match result {
            Ok(x) => Ok(x),
            Err(ref e) if e.kind() == std::io::ErrorKind::WouldBlock => Err(PacketHandlingError::SocketWouldBlock),
            Err(e) => Err(e.into()),
        }
    }

    /// sends a buffer to our peer via the udp socket
    async fn send_buffer_udp(&self, buffer: &[u8]) -> Result<()> {
        match self.udp_peer.as_ref() {
            Some(udp_peer) => self
                .game_server
                .udp_socket
                .send_to(buffer, udp_peer)
                .await
                .map(|_size| ())
                .map_err(PacketHandlingError::SocketSendFailed),

            None => Err(PacketHandlingError::UnableToSendUdp),
        }
    }

    /// non async version of `send_buffer_udp`
    fn send_buffer_udp_immediate(&self, buffer: &[u8]) -> Result<usize> {
        match self.udp_peer.as_ref() {
            Some(udp_peer) => self.game_server.udp_socket.try_send_to(buffer, SocketAddr::V4(*udp_peer)).map_err(|e| {
                if e.kind() == std::io::ErrorKind::WouldBlock {
                    PacketHandlingError::SocketWouldBlock
                } else {
                    PacketHandlingError::SocketSendFailed(e)
                }
            }),

            None => Err(PacketHandlingError::UnableToSendUdp),
        }
    }

    /// fragmented udp send
    async fn send_fragmented_udp_payload(&self, buffer: &[u8]) -> Result<()> {
        let mut vec = vec![0u8; self.mtu];
        let mut buf = FastByteBuffer::new(&mut vec[..]);

        let unique_id: u32 = globed_shared::rand::rng().random();

        // // ok
        let chunk_size = self.mtu - 8;
        let chunk_count = buffer.len().div_ceil(chunk_size);

        if chunk_count > u8::MAX as usize {
            return Err(PacketHandlingError::TooManyChunks(chunk_count));
        }

        for (idx, frame) in buffer.chunks(chunk_size).enumerate() {
            debug_assert!(idx < chunk_count, "too many chunks lol calculation is wrong");

            buf.clear();

            // write marker
            buf.write_u8(MARKER_UDP_FRAME);

            // write unique packet id
            buf.write_u32(unique_id);

            // write frame index
            buf.write_u8(idx as u8);

            // write total frame count
            buf.write_u8(chunk_count as u8);

            // write the data itself
            buf.write_bytes(frame);

            // send itttt
            self.send_buffer_udp(buf.as_bytes()).await?;
        }

        Ok(())
    }
}
