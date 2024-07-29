use super::*;

#[derive(Responder)]
pub struct GenericErrorResponder<T> {
    pub inner: (Status, T),
}

impl<T> From<T> for GenericErrorResponder<String>
where
    T: Error,
{
    fn from(value: T) -> Self {
        GenericErrorResponder {
            inner: (Status::InternalServerError, value.to_string()),
        }
    }
}

pub type WebResult<T> = Result<T, GenericErrorResponder<String>>;
