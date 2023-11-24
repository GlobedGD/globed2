use std::{fmt::Display, time::SystemTimeError};

pub enum PacketHandlingError {
    Other(String),                    // unknown generic error
    WrongCryptoBoxState,              // cryptobox was either Some or None when should've been the other one
    EncryptionError(String),          // failed to encrypt data
    DecryptionError(String),          // failed to decrypt data
    IOError(std::io::Error),          // generic IO error
    MalformedMessage,                 // packet is missing a header
    MalformedLoginAttempt,            // LoginPacket with cleartext credentials
    MalformedCiphertext,              // missing nonce/mac in the encrypted ciphertext
    NoHandler(u16),                   // no handler found for this packet ID
    WebRequestError(reqwest::Error),  // error making a web request to the central server
    UnexpectedPlayerData,             // client sent PlayerDataPacket outside of a level
    SystemTimeError(SystemTimeError), // clock went backwards..?
    SocketSendFailed(std::io::Error), // failed to send data on a socket due to an IO error
    SocketWouldBlock,                 // failed to send data on a socket because operation would block
    UnexpectedCentralResponse,        // malformed response from the central server
}

pub type Result<T> = core::result::Result<T, PacketHandlingError>;

impl From<anyhow::Error> for PacketHandlingError {
    fn from(value: anyhow::Error) -> Self {
        PacketHandlingError::Other(value.to_string())
    }
}

impl From<reqwest::Error> for PacketHandlingError {
    fn from(value: reqwest::Error) -> Self {
        PacketHandlingError::WebRequestError(value)
    }
}

impl From<SystemTimeError> for PacketHandlingError {
    fn from(value: SystemTimeError) -> Self {
        PacketHandlingError::SystemTimeError(value)
    }
}

impl From<std::io::Error> for PacketHandlingError {
    fn from(value: std::io::Error) -> Self {
        PacketHandlingError::IOError(value)
    }
}

impl Display for PacketHandlingError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::Other(msg) => f.write_str(msg),
            Self::IOError(msg) => f.write_fmt(format_args!("IO Error: {msg}")),
            Self::WrongCryptoBoxState => f.write_str("wrong crypto box state for the given operation"),
            Self::EncryptionError(msg) => f.write_fmt(format_args!("Encryption failed: {msg}")),
            Self::DecryptionError(msg) => f.write_fmt(format_args!("Decryption failed: {msg}")),
            Self::MalformedCiphertext => f.write_str("malformed ciphertext in an encrypted packet"),
            Self::MalformedMessage => f.write_str("malformed message structure"),
            Self::MalformedLoginAttempt => f.write_str("malformed login attempt"),
            Self::NoHandler(id) => f.write_fmt(format_args!("no packet handler for packet ID {id}")),
            Self::WebRequestError(msg) => f.write_fmt(format_args!("web request error: {msg}")),
            Self::UnexpectedPlayerData => f.write_str("received PlayerDataPacket when not on a level"),
            Self::SystemTimeError(msg) => f.write_fmt(format_args!("system time error: {msg}")),
            Self::SocketSendFailed(err) => f.write_fmt(format_args!("socket send failed: {err}")),
            Self::SocketWouldBlock => f.write_str("could not do a non-blocking operation on the socket as it would block"),
            Self::UnexpectedCentralResponse => f.write_str("got unexpected response from the central server"),
        }
    }
}
