use super::*;

// client ip address
pub struct CloudflareIPGuard(pub Option<IpAddr>);

#[rocket::async_trait]
impl<'r> FromRequest<'r> for CloudflareIPGuard {
    type Error = &'static str;

    async fn from_request(request: &'r Request<'_>) -> Outcome<Self, Self::Error> {
        match request.headers().get_one("CF-Connecting-IP") {
            Some(x) => match x.parse::<IpAddr>() {
                Ok(a) => Outcome::Success(CloudflareIPGuard(Some(a))),
                Err(_) => Outcome::Error((Status::Unauthorized, "failed to parse the IP header from Cloudflare")),
            },
            None => Outcome::Success(CloudflareIPGuard(None)),
        }
    }
}
