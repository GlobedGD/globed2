use std::{
    error::Error,
    fmt::Display,
    sync::atomic::{AtomicBool, Ordering},
    time::Duration,
};

use esp::{size_of_types, ByteBufferExtRead, ByteBufferExtWrite, ByteReader, DecodeError, FastByteBuffer, StaticSize};
use globed_shared::{
    reqwest::{self, StatusCode},
    GameServerBootData, SyncMutex, TokenIssuer, UserEntry, PROTOCOL_VERSION, SERVER_MAGIC, SERVER_MAGIC_LEN,
};

/// `CentralBridge` stores the configuration of the game server,
/// and is used for making requests to the central server.
pub struct CentralBridge {
    pub http_client: reqwest::Client,
    pub central_url: String,
    pub central_pw: String,
    pub token_issuer: SyncMutex<TokenIssuer>,
    pub central_conf: SyncMutex<GameServerBootData>,

    // for performance reasons /shrug
    pub maintenance: AtomicBool,
    pub whitelist: AtomicBool,
}

#[derive(Debug)]
pub enum CentralBridgeError {
    RequestError(reqwest::Error),       // error making the request
    CentralError((StatusCode, String)), // non 2xx status code
    InvalidMagic(String),               // invalid magic
    MalformedData(DecodeError),         // failed to decode data
    ProtocolMismatch(u16),              // protocol version mismatch
}

impl Display for CentralBridgeError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::RequestError(err) => write!(f, "error making web request: {err}"),
            Self::CentralError((err, response)) => write!(f, "central server returned error {err}: {response}"),
            Self::InvalidMagic(_) => write!(f, "central server sent invalid magic"),
            Self::MalformedData(err) => write!(f, "failed to decode data sent by the central server: {err}"),
            Self::ProtocolMismatch(proto) => write!(
                f,
                "protocol mismatch, we are on v{PROTOCOL_VERSION} while central server is on v{proto}"
            ),
        }
    }
}

impl From<reqwest::Error> for CentralBridgeError {
    fn from(value: reqwest::Error) -> Self {
        Self::RequestError(value)
    }
}

impl From<DecodeError> for CentralBridgeError {
    fn from(value: DecodeError) -> Self {
        Self::MalformedData(value)
    }
}

impl Error for CentralBridgeError {}

pub type Result<T> = std::result::Result<T, CentralBridgeError>;

impl CentralBridge {
    pub fn new(central_url: &str, central_pw: &str) -> Self {
        let http_client = reqwest::Client::builder()
            .timeout(Duration::from_secs(5))
            .user_agent(format!("globed-game-server/{}", env!("CARGO_PKG_VERSION")))
            .build()
            .unwrap();

        Self {
            http_client,
            token_issuer: SyncMutex::new(TokenIssuer::new("", Duration::from_secs(0))),
            central_url: central_url.to_owned(),
            central_pw: central_pw.to_owned(),
            central_conf: SyncMutex::new(GameServerBootData::default()),
            maintenance: AtomicBool::new(false),
            whitelist: AtomicBool::new(false),
        }
    }

    pub fn is_maintenance(&self) -> bool {
        self.maintenance.load(Ordering::Relaxed)
    }

    pub fn is_whitelist(&self) -> bool {
        self.whitelist.load(Ordering::Relaxed)
    }

    pub async fn request_boot_data(&self) -> Result<GameServerBootData> {
        let response = self
            .http_client
            .post(format!("{}gs/boot", self.central_url))
            .query(&[("pw", self.central_pw.clone())])
            .send()
            .await?;

        let status = response.status();
        if status != StatusCode::OK {
            let message = response.text().await.unwrap_or_else(|_| "<no response>".to_owned());

            return Err(CentralBridgeError::CentralError((status, message)));
        }

        let config = response.bytes().await?;
        let mut reader = ByteReader::from_bytes(&config);

        // verify that the magic bytes match
        let valid_magic = reader
            .read_value_array::<u8, SERVER_MAGIC_LEN>()
            .map_or(false, |magic| magic.iter().eq(SERVER_MAGIC.iter()));

        if !valid_magic {
            let txt = String::from_utf8(reader.as_bytes().to_vec()).unwrap_or_else(|_| "<invalid UTF-8 string>".to_owned());
            return Err(CentralBridgeError::InvalidMagic(txt));
        }

        let boot_data = reader.read_value::<GameServerBootData>()?;

        if boot_data.protocol != PROTOCOL_VERSION {
            return Err(CentralBridgeError::ProtocolMismatch(boot_data.protocol));
        }

        Ok(boot_data)
    }

    pub async fn refresh_boot_data(&self) -> Result<()> {
        let data = self.request_boot_data().await?;

        // update various values
        self.token_issuer
            .lock()
            .set_expiration_period(Duration::from_secs(data.token_expiry));

        // set the data
        self.set_boot_data(data);

        Ok(())
    }

    #[inline]
    pub fn set_boot_data(&self, data: GameServerBootData) {
        self.maintenance.store(data.maintenance, Ordering::Relaxed);
        self.whitelist.store(data.whitelist, Ordering::Relaxed);
        let mut issuer = self.token_issuer.lock();

        issuer.set_expiration_period(Duration::from_secs(data.token_expiry));
        issuer.set_secret_key(&data.secret_key2);

        *self.central_conf.lock() = data;
    }

    // other web requests
    pub async fn get_user_data(&self, player: &str) -> Result<UserEntry> {
        let response = self
            .http_client
            .get(format!("{}gs/user/{}", self.central_url, player))
            .query(&[("pw", self.central_pw.clone())])
            .send()
            .await?;

        let status = response.status();
        if status != StatusCode::OK {
            let message = response.text().await.unwrap_or_else(|_| "<no response>".to_owned());

            return Err(CentralBridgeError::CentralError((status, message)));
        }

        let config = response.bytes().await?;
        let mut reader = ByteReader::from_bytes(&config);
        reader.validate_self_checksum()?;

        Ok(reader.read_value::<UserEntry>()?)
    }

    pub async fn update_user_data(&self, user: &UserEntry) -> Result<()> {
        let mut rawbuf = [0u8; UserEntry::ENCODED_SIZE + size_of_types!(u32)];
        let mut buffer = FastByteBuffer::new(&mut rawbuf);

        buffer.write_value(user);
        buffer.append_self_checksum();

        let body = buffer.to_vec();

        let response = self
            .http_client
            .post(format!("{}gs/user/update", self.central_url))
            .query(&[("pw", self.central_pw.clone())])
            .body(body)
            .send()
            .await?;

        let status = response.status();
        if status != StatusCode::OK {
            let message = response.text().await.unwrap_or_else(|_| "<no response>".to_owned());

            return Err(CentralBridgeError::CentralError((status, message)));
        }

        Ok(())
    }
}
