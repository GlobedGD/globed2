use darling::FromDeriveInput;
use syn::DeriveInput;

#[derive(FromDeriveInput, Copy, Clone)]
#[darling(attributes(bitfield))]
pub(crate) struct BitfieldAttributes {
    pub on: Option<bool>,
    pub size: Option<usize>,
}

impl BitfieldAttributes {
    pub fn parse(input: &DeriveInput) -> Option<Self> {
        let res = Self::from_derive_input(input).expect("Failed to parse bitfield attributes");
        if res.on.unwrap_or(false) { Some(res) } else { None }
    }
}

#[derive(FromDeriveInput, Copy, Clone)]
#[darling(attributes(dynamic_size))]
pub(crate) struct DynamicSizeAttributes {
    pub as_static: Option<bool>,
}

#[derive(FromDeriveInput, Copy, Clone)]
#[darling(attributes(packet))]
pub(crate) struct PacketAttributes {
    pub id: u16,
    pub encrypted: Option<bool>,
    pub tcp: Option<bool>,
}
