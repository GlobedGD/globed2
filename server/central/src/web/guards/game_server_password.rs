use super::*;

// game server password guar
pub struct GameServerPasswordGuard(pub String);

#[rocket::async_trait]
impl<'r> FromRequest<'r> for GameServerPasswordGuard {
    type Error = &'static str;

    async fn from_request(request: &'r Request<'_>) -> Outcome<Self, Self::Error> {
        match request.headers().get_one("Authorization") {
            Some(x) => Outcome::Success(GameServerPasswordGuard(x.to_owned())),
            None => Outcome::Error((Status::Unauthorized, "no password provided")),
        }
    }
}

impl GameServerPasswordGuard {
    pub fn verify(&self, correct: &str) -> bool {
        if self.0.len() != correct.len() {
            return false;
        }

        let bytes1 = self.0.as_bytes();
        let bytes2 = correct.as_bytes();

        let mut result = 0u8;
        for (b1, b2) in bytes1.iter().zip(bytes2.iter()) {
            result |= b1 ^ b2;
        }

        result == 0
    }
}
