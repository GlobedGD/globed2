use std::error::Error;

use rocket::{
    Request,
    http::Status,
    response::{Responder, Response},
};

pub mod maintenance;
pub use maintenance::MaintenanceResponder;

pub mod generic;
pub use generic::{GenericErrorResponder, WebResult};

pub mod unauthorized_;
pub use unauthorized_::UnauthorizedResponder;
pub(crate) use unauthorized_::unauthorized;

pub mod bad_request_;
pub use bad_request_::BadRequestResponder;
pub(crate) use bad_request_::bad_request;

pub mod not_found_;
pub use not_found_::NotFoundResponder;
pub(crate) use not_found_::not_found;

pub mod encodable;
pub use encodable::{CheckedEncodableResponder, EncodableResponder};
