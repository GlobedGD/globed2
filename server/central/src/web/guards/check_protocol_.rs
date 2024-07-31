use super::*;

macro_rules! check_protocol {
    ($protocol:expr) => {
        let p = $protocol;

        if !globed_shared::SUPPORTED_PROTOCOLS.contains(&p) && p != 0xffff {
            if p > globed_shared::MAX_SUPPORTED_PROTOCOL {
                bad_request!("Your Globed version is too new. This server is outdated, and does not support your client version.");
            }

            bad_request!(&format!(
                "Outdated client, please update Globed. This server requires at least version {MIN_CLIENT_VERSION}"
            ));
        }
    };
}

pub(crate) use check_protocol;
