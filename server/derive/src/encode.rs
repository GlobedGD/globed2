use proc_macro2::TokenStream;
use quote::quote;
use syn::{DataStruct, Ident, ImplGenerics, TypeGenerics, WhereClause};

use crate::{attrs::BitfieldAttributes, get_bitfield_struct_size, is_field_bool};

pub(crate) fn for_struct(
    data: &DataStruct,
    struct_name: &Ident,
    impl_generics: &ImplGenerics,
    ty_generics: &TypeGenerics,
    where_clause: Option<&WhereClause>,
) -> TokenStream {
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
        use esp::ByteBufferExtWrite as _;
        impl #impl_generics Encodable for #struct_name #ty_generics #where_clause {
            #[inline]
            fn encode(&self, buf: &mut esp::ByteBuffer) {
                #(#encode_fields)*
            }
            #[inline]
            fn encode_fast(&self, buf: &mut esp::FastByteBuffer) {
                #(#encode_fields)*
            }
        }
    }
}

pub(crate) fn for_bit_struct(
    data: &DataStruct,
    bit_attrs: BitfieldAttributes,
    struct_name: &Ident,
    impl_generics: &ImplGenerics,
    ty_generics: &TypeGenerics,
    where_clause: Option<&WhereClause>,
) -> TokenStream {
    let size = bit_attrs.size.unwrap_or_else(|| get_bitfield_struct_size(data));

    let encode_fields: Vec<_> = data
        .fields
        .iter()
        .enumerate()
        .map(|(idx, field)| {
            let Some(ident) = field.ident.as_ref() else {
                return quote! {
                    compile_error!("Encodable cannot be derived for tuple structs");
                };
            };

            if !is_field_bool(&field.ty) {
                return quote! {
                    compile_error!("Non-bool type found in a bit-field struct");
                };
            }

            quote! {
                bits.assign_bit(#idx, self.#ident);
            }
        })
        .collect();

    let encode_body = quote! {
        use esp::Bits;

        let mut bits = Bits::<#size>::new();
        #(#encode_fields)*

        buf.write_value(&bits);
    };

    quote! {
        use esp::ByteBufferExtWrite as _;
        impl #impl_generics Encodable for #struct_name #ty_generics #where_clause {
            #[inline]
            fn encode(&self, buf: &mut esp::ByteBuffer) {
                #encode_body
            }

            #[inline]
            fn encode_fast(&self, buf: &mut esp::FastByteBuffer) {
                #encode_body
            }
        }
    }
}

pub(crate) fn for_enum(
    name: &Ident,
    repr_type: &TokenStream,
    impl_generics: &ImplGenerics,
    ty_generics: &TypeGenerics,
    where_clause: Option<&WhereClause>,
) -> TokenStream {
    quote! {
        use esp::ByteBufferExtWrite as _;
        impl #impl_generics Encodable for #name #ty_generics #where_clause {
            #[inline]
            fn encode(&self, buf: &mut esp::ByteBuffer) {
                buf.write_value(&(*self as #repr_type))
            }

            #[inline]
            fn encode_fast(&self, buf: &mut esp::FastByteBuffer) {
                buf.write_value(&(*self as #repr_type))
            }
        }
    }
}
