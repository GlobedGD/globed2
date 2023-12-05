#![feature(proc_macro_diagnostic)]
#![allow(clippy::missing_panics_doc)]

use darling::FromDeriveInput;
use proc_macro::{self, Span, TokenStream};
use quote::{quote, ToTokens};
use syn::{parse_macro_input, punctuated::Punctuated, Data, DeriveInput, Meta, Token};

/// Implements `Encodable` for the given type, allowing you to serialize it into a regular `ByteBuffer`.
/// For `Encodable` to be successfully derived, for structs, all of the members of the struct must also implement `Encodable`.
/// The members are serialized in the same order they are laid out in the struct.
///
/// For enums, the enum must derive `Copy`, must be plain (no associated data fields), and may have a
/// `#[repr(u*)]` or `#[repr(i*)]` attribute to indicate the encoded type. By default it will be `i32` if omitted.
#[proc_macro_derive(Encodable)]
pub fn derive_encodable(input: TokenStream) -> TokenStream {
    let input = parse_macro_input!(input as DeriveInput);

    let struct_name = &input.ident;

    let gen = match &input.data {
        Data::Struct(data) => {
            let encode_fields: Vec<_> = data
                .fields
                .iter()
                .map(|field| {
                    let Some(ident) = field.ident.as_ref() else {
                        return quote! {
                            compile_error!("Encodable cannot be derived for tuple structs");
                        };
                    };

                    quote! {
                        buf.write_value(&self.#ident);
                    }
                })
                .collect();

            quote! {
                impl Encodable for #struct_name {
                    fn encode(&self, buf: &mut bytebuffer::ByteBuffer) {
                        #(#encode_fields)*
                    }

                    fn encode_fast(&self, buf: &mut crate::data::FastByteBuffer) {
                        #(#encode_fields)*
                    }
                }
            }
        }
        Data::Enum(_) => {
            let repr_type = get_enum_repr_type(&input);

            quote! {
                impl Encodable for #struct_name {
                    fn encode(&self, buf: &mut bytebuffer::ByteBuffer) {
                        buf.write_value(&(*self as #repr_type))
                    }

                    fn encode_fast(&self, buf: &mut crate::data::FastByteBuffer) {
                        buf.write_value(&(*self as #repr_type))
                    }
                }
            }
        }
        Data::Union(_) => {
            return quote! {
                compile_error!("Encodable cannot be derived for unions");
            }
            .into()
        }
    };

    gen.into()
}

/// Implements `KnownSize` for the given type, allowing you to serialize it into a `FastByteBuffer`.
/// For `KnownSize` to be successfully derived, for structs, all of the members of the struct must also implement `KnownSize`.
///
/// For enums, all the same limitations apply as in `Encodable`.
#[proc_macro_derive(KnownSize)]
pub fn derive_known_size(input: TokenStream) -> TokenStream {
    let input = parse_macro_input!(input as DeriveInput);

    let struct_name = &input.ident;

    let encoded_size = match &input.data {
        Data::Struct(data) if data.fields.is_empty() => {
            // If the struct has no fields, encoded size is 0
            quote! { 0 }
        }
        Data::Struct(data) => {
            let field_types: Vec<_> = data.fields.iter().map(|field| &field.ty).collect();

            quote! {
                size_of_types!(#(#field_types),*)
            }
        }
        Data::Enum(_) => {
            let repr_type = get_enum_repr_type(&input);

            quote! {
                std::mem::size_of::<#repr_type>()
            }
        }
        Data::Union(_) => {
            return quote! {
                compile_error!("KnownSize cannot be drived for unions");
            }
            .into();
        }
    };

    let gen = quote! {
        impl KnownSize for #struct_name {
            const ENCODED_SIZE: usize = #encoded_size;
        }
    };

    // Return the generated implementation as a TokenStream
    gen.into()
}

/// Implements `Decodable` for the given type, allowing you to deserialize it from a `ByteReader`/`ByteBuffer`.
/// For `Decodable` to be successfully derived, for structs, all of the members of the struct must also implement `Decodable`.
/// The members are deserialized in the same order they are laid out in the struct.
///
/// For enums, all the same limitations apply as in `Encodable` plus the enum must have explicitly specified values for all variants.
#[proc_macro_derive(Decodable)]
pub fn derive_decodable(input: TokenStream) -> TokenStream {
    let input = parse_macro_input!(input as DeriveInput);

    let struct_name = &input.ident;

    let gen = match &input.data {
        Data::Struct(data) => {
            let field_names = data.fields.iter().map(|field| field.ident.as_ref().unwrap());

            let decode_fields = field_names
                .clone()
                .map(|ident| {
                    quote! {
                        let #ident = buf.read()?;
                    }
                })
                .collect::<Vec<_>>();

            let field_names: Vec<_> = field_names.collect();

            quote! {
                impl Decodable for #struct_name {
                    fn decode(buf: &mut bytebuffer::ByteBuffer) -> crate::data::DecodeResult<Self> {
                        #(#decode_fields)*
                        Ok(Self {
                            #(
                                #field_names,
                            )*
                        })
                    }

                    fn decode_from_reader(buf: &mut bytebuffer::ByteReader) -> crate::data::DecodeResult<Self> {
                        #(#decode_fields)*
                        Ok(Self {
                            #(
                                #field_names,
                            )*
                        })
                    }
                }
            }
        }
        Data::Enum(data) => {
            let repr_type = get_enum_repr_type(&input);

            let mut err_flag: bool = false;
            let decode_variants: Vec<_> = data
                .variants
                .iter()
                .map(|variant| {
                    let ident = &variant.ident;
                    let variant_repr = match &variant.discriminant {
                        Some((_, expr)) => {
                            quote! { #expr }
                        }
                        None => {
                            err_flag = true;
                            quote! { 0 }
                        }
                    };
                    quote! {
                        #variant_repr => Ok(#struct_name::#ident),
                    }
                })
                .collect();

            if err_flag {
                return quote! {
                    compile_error!("Decodable currently cannot be derived for enums without explicitly specified discriminants");
                }
                .into();
            }

            quote! {
                impl Decodable for #struct_name {
                    fn decode(buf: &mut bytebuffer::ByteBuffer) -> crate::data::DecodeResult<Self> {
                        let value: #repr_type = buf.read_value()?;
                        match value {
                            #(#decode_variants)*
                            _ => Err(crate::data::DecodeError::InvalidEnumValue)
                        }
                    }

                    fn decode_from_reader(buf: &mut bytebuffer::ByteReader) -> crate::data::DecodeResult<Self> {
                        let value: #repr_type = buf.read_value()?;
                        match value {
                            #(#decode_variants)*
                            _ => Err(crate::data::DecodeError::InvalidEnumValue)
                        }
                    }
                }
            }
        }
        Data::Union(_) => {
            return quote! {
                compile_error!("Decodable cannot be drived for unions");
            }
            .into();
        }
    };

    gen.into()
}

fn get_enum_repr_type(input: &DeriveInput) -> proc_macro2::TokenStream {
    let mut repr_type: Option<proc_macro2::TokenStream> = None;
    for attr in &input.attrs {
        if attr.path().is_ident("repr") {
            let nested = attr.parse_args_with(Punctuated::<Meta, Token![,]>::parse_terminated).unwrap();
            for meta in nested {
                match meta {
                    Meta::Path(path) => {
                        if let Some(ident) = path.get_ident() {
                            repr_type = Some(ident.to_token_stream());
                        }
                    }
                    _ => {
                        return TokenStream::from(quote! {
                            compile_error!("unrecognized repr attribute");
                        })
                        .into();
                    }
                }
            }
        }
    }

    if repr_type.is_none() {
        // if not specified, assume i32 and give a warning.
        repr_type = Some(quote! { i32 });
        Span::call_site().warning("enum repr type not specified - assuming i32. it is recommended to add #[repr(type)] before the enum as that makes it more explicit.").emit();
    }

    repr_type.unwrap()
}

#[derive(FromDeriveInput)]
#[darling(attributes(packet))]
struct PacketAttributes {
    id: u16,
    encrypted: bool,
}

/// Implements `Packet`, `PacketMetadata` and the function `header() -> PacketHeader` for the given struct.
/// You must also pass additional attributes with `#[packet]`, specifically packet ID and whether the packet should be encrypted.
/// Example:
/// ```rust
/// #[derive(Packet, Encodable, Decodable)]
/// #[packet(id = 10000, encrypted = false)]
/// pub struct MyPacket { /* fields */ }
/// ```
#[proc_macro_derive(Packet, attributes(packet))]
pub fn packet(input: TokenStream) -> TokenStream {
    let input = parse_macro_input!(input);
    let Ok(opts) = PacketAttributes::from_derive_input(&input) else {
        return quote! {
            compile_error!("invalid or missing signature for #[packet] attribute, please see documentation for `Packet` proc macro");
        }
        .into();
    };

    let DeriveInput { ident, .. } = input;

    let id = opts.id;
    let enc = opts.encrypted;

    let output = match &input.data {
        Data::Struct(_) => {
            quote! {
                impl PacketMetadata for #ident {
                    const PACKET_ID: crate::data::packets::PacketId = #id;
                    const ENCRYPTED: bool = #enc;
                    const NAME: &'static str = stringify!(#ident);
                }

                impl Packet for #ident {}

                impl #ident {
                    pub const fn header() -> crate::data::packets::PacketHeader {
                        crate::data::packets::PacketHeader::from_packet::<Self>()
                    }
                }
            }
        }
        Data::Enum(_) | Data::Union(_) => {
            quote! {
                compile_error!("Packet cannot be derived for enums or unions");
            }
        }
    };

    output.into()
}
