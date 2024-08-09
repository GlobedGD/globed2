use super::*;

#[derive(Responder)]
#[response(status = 503, content_type = "text")]
pub struct MaintenanceResponder {
    pub inner: &'static str,
}

impl From<MaintenanceResponder> for GenericErrorResponder<String> {
    fn from(value: MaintenanceResponder) -> Self {
        GenericErrorResponder {
            inner: (Status::ServiceUnavailable, value.inner.to_string()),
        }
    }
}
