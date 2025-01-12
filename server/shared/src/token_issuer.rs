use std::{
    fmt::Display,
    time::{Duration, SystemTime, UNIX_EPOCH},
};

use base64::{Engine, engine::general_purpose as b64e};
use hmac::{Hmac, KeyInit, Mac};
use sha2::Sha256;

pub struct TokenIssuer {
    hmac: Hmac<Sha256>,
    expiration_period: Duration,
}

pub enum TokenValidationFailure {
    Missing,            // token string is empty
    MalformedStructure, // malformed token structure
    Impersonation,      // account IDs don't match
    Expired,            // token expired
    InvalidSignature,   // signature does not match
}

type HmacSha256 = Hmac<Sha256>;

impl TokenValidationFailure {
    pub const fn error_message(&self) -> &'static str {
        match self {
            Self::Missing => "no token provided",
            Self::MalformedStructure => "malformed token structure",
            Self::Impersonation => "account ID does not match this token",
            Self::Expired => "token expired",
            Self::InvalidSignature => "signature mismatch",
        }
    }
}

impl Display for TokenValidationFailure {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str(self.error_message())
    }
}

impl TokenIssuer {
    pub fn new(secret_key: &str, expiration_period: Duration) -> Self {
        let skey_bytes = secret_key.as_bytes();
        let hmac = HmacSha256::new_from_slice(skey_bytes).unwrap();

        Self { hmac, expiration_period }
    }

    /// Change the secret key of this token issuer.
    pub fn set_secret_key(&mut self, secret_key: &str) {
        let skey_bytes = secret_key.as_bytes();
        let hmac = HmacSha256::new_from_slice(skey_bytes).unwrap();

        self.hmac = hmac;
    }

    pub fn set_expiration_period(&mut self, period: Duration) {
        self.expiration_period = period;
    }

    /// Generates a new session token for the given account ID and username
    pub fn generate(&self, account_id: i32, user_id: i32, account_name: &str) -> String {
        let timestamp = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .expect("whoops our clock went backwards")
            .as_secs();

        let data = format!("{account_id}.{user_id}.{account_name}.{timestamp}");
        let mut hmac = self.hmac.clone();
        hmac.update(data.as_bytes());
        let res = hmac.finalize();

        format!(
            "{}.{}",
            b64e::URL_SAFE_NO_PAD.encode(data),
            b64e::URL_SAFE_NO_PAD.encode(res.into_bytes())
        )
    }

    /// Validates a token, returns the name of the user if successful
    pub fn validate(&self, account_id: i32, user_id: i32, token: &str) -> Result<String, TokenValidationFailure> {
        if token.is_empty() {
            return Err(TokenValidationFailure::Missing);
        }

        let timestamp = SystemTime::now();

        let (claims, signature) = token.split_once('.').ok_or(TokenValidationFailure::MalformedStructure)?;

        let data_str = String::from_utf8(
            b64e::URL_SAFE_NO_PAD
                .decode(claims)
                .map_err(|_| TokenValidationFailure::MalformedStructure)?,
        )
        .map_err(|_| TokenValidationFailure::MalformedStructure)?;

        let mut claims = data_str.split('.');

        let orig_id = claims
            .next()
            .ok_or(TokenValidationFailure::MalformedStructure)?
            .parse::<i32>()
            .map_err(|_| TokenValidationFailure::MalformedStructure)?;

        let orig_user_id = claims
            .next()
            .ok_or(TokenValidationFailure::MalformedStructure)?
            .parse::<i32>()
            .map_err(|_| TokenValidationFailure::MalformedStructure)?;

        let orig_name = claims.next().ok_or(TokenValidationFailure::MalformedStructure)?;

        let orig_ts = claims
            .next()
            .ok_or(TokenValidationFailure::MalformedStructure)?
            .parse::<u64>()
            .map_err(|_| TokenValidationFailure::MalformedStructure)?;

        if orig_id != account_id {
            return Err(TokenValidationFailure::Impersonation);
        }

        if orig_user_id != user_id {
            return Err(TokenValidationFailure::Impersonation);
        }

        let elapsed = timestamp
            .duration_since(UNIX_EPOCH + Duration::from_secs(orig_ts))
            .map_err(|_| TokenValidationFailure::MalformedStructure)?;

        if elapsed > self.expiration_period {
            return Err(TokenValidationFailure::Expired);
        }

        // verify the signature
        let mut hmac = self.hmac.clone();
        hmac.update(data_str.as_bytes());

        let signature = b64e::URL_SAFE_NO_PAD
            .decode(signature)
            .map_err(|_| TokenValidationFailure::MalformedStructure)?;

        hmac.verify_slice(&signature).map_err(|_| TokenValidationFailure::InvalidSignature)?;

        Ok(orig_name.to_string())
    }
}
