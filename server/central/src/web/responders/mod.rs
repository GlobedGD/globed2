use std::error::Error;

use rocket::{
    http::Status,
    response::{Responder, Response},
    Request,
};

pub mod maintenance;
pub use maintenance::MaintenanceResponder;

pub mod generic;
pub use generic::{GenericErrorResponder, WebResult};

pub mod unauthorized_;
pub(crate) use unauthorized_::unauthorized;
pub use unauthorized_::UnauthorizedResponder;

pub mod bad_request_;
pub(crate) use bad_request_::bad_request;
pub use bad_request_::BadRequestResponder;

pub mod not_found_;
pub(crate) use not_found_::not_found;
pub use not_found_::NotFoundResponder;

pub mod encodable;
pub use encodable::{CheckedEncodableResponder, EncodableResponder};
