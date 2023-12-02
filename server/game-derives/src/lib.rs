/*
* proc macros for Encodable, Decodable, EncodableWithKnownSize, and Packet traits.
* Using the first three is as simple as `#[derive(Encodable, ...)]`, Packet is a bit more complex:
* #[derive(Packet)]
* #[packet(id = 10000, encrypted = false)]
* pub struct MyPacket {}
*/

#![allow(clippy::missing_panics_doc)]

use darling::FromDeriveInput;
use proc_macro::{self, TokenStream};
use quote::quote;
use syn::{parse_macro_input, Data, DeriveInput};

/* Encodable, EncodableWithKnownSize, and Decodable derive macros */

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
                    let ident = field.ident.as_ref().unwrap();
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
        _ => {
            return TokenStream::from(quote! {
                compile_error!("Encodable can only be derived for structs");
            })
        }
    };

    gen.into()
}

#[proc_macro_derive(EncodableWithKnownSize)]
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
        _ => {
            return TokenStream::from(quote! {
                compile_error!("EncodableWithKnownSize can only be derived for structs");
            });
        }
    };

    let gen = quote! {
        impl EncodableWithKnownSize for #struct_name {
            const ENCODED_SIZE: usize = #encoded_size;
        }
    };

    // Return the generated implementation as a TokenStream
    gen.into()
}

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
                    fn decode(buf: &mut bytebuffer::ByteBuffer) -> anyhow::Result<Self> {
                        #(#decode_fields)*
                        Ok(Self {
                            #(
                                #field_names,
                            )*
                        })
                    }

                    fn decode_from_reader(buf: &mut bytebuffer::ByteReader) -> anyhow::Result<Self> {
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
        _ => {
            return TokenStream::from(quote! {
                compile_error!("Decodable can only be derived for structs");
            })
        }
    };

    gen.into()
}

/* #[packet()] implementation */

#[derive(FromDeriveInput)]
#[darling(attributes(packet))]
struct PacketAttributes {
    id: u16,
    encrypted: bool,
}

#[proc_macro_derive(Packet, attributes(packet))]
pub fn packet(input: TokenStream) -> TokenStream {
    let input = parse_macro_input!(input);
    let opts = PacketAttributes::from_derive_input(&input).expect("wrong value passed into #[packet] derive macro");
    let DeriveInput { ident, .. } = input;

    let id = opts.id;
    let enc = opts.encrypted;

    let output = quote! {
        impl PacketMetadata for #ident {
            const PACKET_ID: crate::data::packets::PacketId = #id;
            const ENCRYPTED: bool = #enc;
            const NAME: &'static str = stringify!(#ident);
        }

        impl Packet for #ident {
            fn get_packet_id(&self) -> crate::data::packets::PacketId {
                #id
            }

            fn get_encrypted(&self) -> bool {
                #enc
            }
        }

        impl #ident {
            pub const fn header() -> crate::data::packets::PacketHeader {
                crate::data::packets::PacketHeader::from_packet::<Self>()
            }
        }
    };

    output.into()
}
