use proc_macro2::TokenStream;
use quote::quote;
use syn::{DataEnum, DataStruct, Ident};

use crate::{attrs::BitfieldAttributes, get_bitfield_struct_size};

pub(crate) fn for_struct(data: &DataStruct, struct_name: &Ident) -> TokenStream {
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

    let decode_body = quote! {
        #(#decode_fields)*
        Ok(Self {
            #(
                #field_names,
            )*
        })
    };

    quote! {
        use esp::ByteBufferExtRead as _;
        impl Decodable for #struct_name {
            #[inline]
            fn decode(buf: &mut esp::ByteBuffer) -> esp::DecodeResult<Self> {
                #decode_body
            }

            #[inline]
            fn decode_from_reader(buf: &mut esp::ByteReader) -> esp::DecodeResult<Self> {
                #decode_body
            }
        }
    }
}

pub(crate) fn for_bit_struct(data: &DataStruct, bit_attrs: BitfieldAttributes, struct_name: &Ident) -> TokenStream {
    let size = bit_attrs.size.unwrap_or_else(|| get_bitfield_struct_size(data));

    let field_names = data.fields.iter().map(|field| field.ident.as_ref().unwrap());

    let decode_fields = field_names
        .clone()
        .enumerate()
        .map(|(idx, ident)| {
            quote! {
                let #ident = bits.get_bit(#idx);
            }
        })
        .collect::<Vec<_>>();

    let field_names: Vec<_> = field_names.collect();

    let decode_body = quote! {
        let bits = buf.read_value::<Bits<#size>>()?;

        #(#decode_fields)*
        Ok(Self {
            #(
                #field_names,
            )*
        })
    };

    quote! {
        use esp::ByteBufferExtRead as _;
        impl Decodable for #struct_name {
            #[inline]
            fn decode(buf: &mut esp::ByteBuffer) -> esp::DecodeResult<Self> {
                #decode_body
            }

            #[inline]
            fn decode_from_reader(buf: &mut esp::ByteReader) -> esp::DecodeResult<Self> {
                #decode_body
            }
        }

    }
}

pub(crate) fn for_enum(data: &DataEnum, name: &Ident, repr_type: &TokenStream) -> TokenStream {
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
                #variant_repr => Ok(#name::#ident),
            }
        })
        .collect();

    if err_flag {
        return quote! {
            compile_error!("Decodable currently cannot be derived for enums without explicitly specified discriminants");
        };
    }

    quote! {
        use esp::ByteBufferExtRead as _;
        impl Decodable for #name {
            #[inline]
            fn decode(buf: &mut esp::ByteBuffer) -> esp::DecodeResult<Self> {
                let value: #repr_type = buf.read_value()?;
                match value {
                    #(#decode_variants)*
                    _ => Err(esp::DecodeError::InvalidEnumValue)
                }
            }
            #[inline]
            fn decode_from_reader(buf: &mut esp::ByteReader) -> esp::DecodeResult<Self> {
                let value: #repr_type = buf.read_value()?;
                match value {
                    #(#decode_variants)*
                    _ => Err(esp::DecodeError::InvalidEnumValue)
                }
            }
        }
    }
}
