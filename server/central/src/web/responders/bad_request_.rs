use super::*;

#[derive(Responder)]
#[response(status = 400, content_type = "text")]
pub struct BadRequestResponder {
    pub inner: String,
}

impl BadRequestResponder {
    pub fn new(inner: &str) -> Self {
        Self { inner: inner.to_owned() }
    }
}

impl From<BadRequestResponder> for GenericErrorResponder<String> {
    fn from(value: BadRequestResponder) -> Self {
        GenericErrorResponder {
            inner: (Status::BadRequest, value.inner),
        }
    }
}

macro_rules! bad_request {
    ($msg:expr) => {
        return Err(BadRequestResponder::new($msg).into())
    };
}

pub(crate) use bad_request;
