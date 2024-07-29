use super::*;

#[derive(Responder)]
#[response(status = 404, content_type = "text")]
pub struct NotFoundResponder {
    pub inner: String,
}

impl NotFoundResponder {
    pub fn new(inner: String) -> Self {
        Self { inner }
    }
}

impl From<NotFoundResponder> for GenericErrorResponder<String> {
    fn from(value: NotFoundResponder) -> Self {
        GenericErrorResponder {
            inner: (Status::NotFound, value.inner),
        }
    }
}
macro_rules! not_found {
    ($msg:expr) => {
        return Err(NotFoundResponder::new($msg).into())
    };
}

pub(crate) use not_found;
