use super::*;

#[derive(Responder)]
#[response(status = 401, content_type = "text")]
pub struct UnauthorizedResponder {
    pub inner: String,
}

impl UnauthorizedResponder {
    pub fn new(inner: &str) -> Self {
        Self { inner: inner.to_owned() }
    }
}

impl From<UnauthorizedResponder> for GenericErrorResponder<String> {
    fn from(value: UnauthorizedResponder) -> Self {
        GenericErrorResponder {
            inner: (Status::Unauthorized, value.inner),
        }
    }
}

macro_rules! unauthorized {
    ($msg:expr) => {
        return Err(UnauthorizedResponder::new($msg).into())
    };
}

pub(crate) use unauthorized;
