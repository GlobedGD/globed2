use super::*;

// game server user agent
pub struct GameServerUserAgentGuard<'r>(pub &'r str);

#[rocket::async_trait]
impl<'r> FromRequest<'r> for GameServerUserAgentGuard<'r> {
    type Error = &'static str;

    async fn from_request(request: &'r Request<'_>) -> Outcome<Self, Self::Error> {
        match request.headers().get_one("User-Agent") {
            Some(ua) if ua.starts_with("globed-game-server") => Outcome::Success(GameServerUserAgentGuard(ua)),
            Some(_) => Outcome::Error((Status::Unauthorized, "bad request?")),
            None => Outcome::Error((Status::Unauthorized, "missing useragent")),
        }
    }
}
