use super::*;

// client user agent
pub struct ClientUserAgentGuard<'r>(pub &'r str);

#[rocket::async_trait]
impl<'r> FromRequest<'r> for ClientUserAgentGuard<'r> {
    type Error = &'static str;

    async fn from_request(request: &'r Request<'_>) -> Outcome<Self, Self::Error> {
        match request.headers().get_one("User-Agent") {
            Some(ua) if ua.starts_with("globed-geode-xd") => Outcome::Success(ClientUserAgentGuard(ua)),
            Some(_) => Outcome::Error((Status::Unauthorized, "bad request?")),
            None => Outcome::Error((Status::Unauthorized, "missing useragent")),
        }
    }
}
