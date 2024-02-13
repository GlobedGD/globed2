#![feature(proc_macro_diagnostic)]
#![allow(clippy::missing_panics_doc)]

use darling::FromDeriveInput;
use proc_macro::{self, Span, TokenStream};
use quote::{quote, ToTokens};
use syn::{parse_macro_input, punctuated::Punctuated, Data, DeriveInput, Meta, Token};

/// Implements `Encodable` for the given type, allowing you to serialize it into a `ByteBuffer` or a `FastByteBuffer`.
/// For `Encodable` to be successfully derived, for structs, all of the members of the struct must also implement `Encodable`.
/// The members are serialized in the same order they are laid out in the struct.
///
/// For enums, the enum must derive `Copy`, must be plain (no associated data fields), and may have a
/// `#[repr(u*)]` or `#[repr(i*)]` attribute to indicate the encoded type. By default it will be `i32` if omitted.
#[proc_macro_derive(Encodable)]
pub fn derive_encodable(input: TokenStream) -> TokenStream {
    let input = parse_macro_input!(input as DeriveInput);

    let struct_name = &input.ident;
    let (impl_generics, ty_generics, where_clause) = input.generics.split_for_impl();

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
        Data::Enum(_) => {
            let repr_type = get_enum_repr_type(&input);

            quote! {
                use esp::ByteBufferExtWrite as _;
                impl #impl_generics Encodable for #struct_name #ty_generics #where_clause {
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
        Data::Union(_) => {
            return quote! {
                compile_error!("Encodable cannot be derived for unions");
            }
            .into()
        }
    };

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
                use esp::ByteBufferExtRead as _;
                impl Decodable for #struct_name {
                    #[inline]
                    fn decode(buf: &mut esp::ByteBuffer) -> esp::DecodeResult<Self> {
                        #(#decode_fields)*
                        Ok(Self {
                            #(
                                #field_names,
                            )*
                        })
                    }
                    #[inline]
                    fn decode_from_reader(buf: &mut esp::ByteReader) -> esp::DecodeResult<Self> {
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
                use esp::ByteBufferExtRead as _;
                impl Decodable for #struct_name {
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
        Data::Union(_) => {
            return quote! {
                compile_error!("Decodable cannot be drived for unions");
            }
            .into();
        }
    };

    gen.into()
}

/// Implements `StaticSize` for the given type, allowing you to compute the encoded size of the value at compile time.
/// For `StaticSize` to be successfully derived, for structs, all of the members of the struct must also implement `StaticSize`.
///
/// For enums, all the same limitations apply as in `Encodable`.
#[proc_macro_derive(StaticSize)]
pub fn derive_static_size(input: TokenStream) -> TokenStream {
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
                esp::size_of_types!(#(#field_types),*)
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
                compile_error!("StaticSize cannot be drived for unions");
            }
            .into();
        }
    };

    let gen = quote! {
        impl StaticSize for #struct_name {
            const ENCODED_SIZE: usize = #encoded_size;
        }
    };

    // Return the generated implementation as a TokenStream
    gen.into()
}

#[derive(FromDeriveInput)]
#[darling(attributes(dynamic_size))]
struct DynamicSizeAttributes {
    as_static: Option<bool>,
}

/// Implements `DynamicSize` for the given type, allowing you to compute the encoded size of the value at runtime.
/// For `DynamicSize` to be successfully derived, for structs, all of the members of the struct must also implement `DynamicSize`.
///
/// For enums, all the same limitations apply as in `Encodable`.
///
/// If your struct also implements `StaticSize`, for efficiency purposes you can add the attribute `#[dynamic_size(as_static = true)]`, like so:
/// ```rust
/// #[derive(StaticSize, DynamicSize)]
/// #[dynamic_size(as_static = true)]
/// pub struct MyStruct { val: u32 }
/// ```
///
/// This will cause `MyStruct::encoded_size` to evaluate to a constant `u32::ENCODED_SIZE` instead of the function call `self.val.encoded_size()`
#[proc_macro_derive(DynamicSize, attributes(dynamic_size))]
pub fn derive_dynamic_size(input: TokenStream) -> TokenStream {
    let input = parse_macro_input!(input as DeriveInput);
    let as_static = match DynamicSizeAttributes::from_derive_input(&input) {
        Ok(x) => x.as_static,
        Err(_) => {
            return quote! {
                compile_error!("invalid signature for #[dynamic_size] attribute, please see documentation for `DynamicSize` proc macro");
            }
            .into();
        }
    }.unwrap_or(false);

    let struct_name = &input.ident;
    let (impl_generics, ty_generics, where_clause) = input.generics.split_for_impl();

    let encoded_size = match &input.data {
        Data::Struct(data) if data.fields.is_empty() => {
            // If the struct has no fields, encoded size is 0
            quote! { 0 }
        }
        Data::Struct(data) => {
            let field_names: Vec<_> = data.fields.iter().map(|field| &field.ident).collect();

            if as_static {
                quote! {
                    Self::ENCODED_SIZE
                }
            } else {
                quote! {
                    esp::size_of_dynamic_types!(#(&self.#field_names),*)
                }
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
                compile_error!("DynamicSize cannot be drived for unions");
            }
            .into();
        }
    };

    let gen = quote! {
        impl #impl_generics DynamicSize for #struct_name #ty_generics #where_clause {
            #[inline]
            fn encoded_size(&self) -> usize {
                #encoded_size
            }
        }
    };

    // Return the generated implementation as a TokenStream
    gen.into()
}

#[derive(FromDeriveInput)]
#[darling(attributes(packet))]
struct PacketAttributes {
    id: u16,
    encrypted: Option<bool>,
    tcp: Option<bool>,
}

/// Implements `Packet`, `PacketMetadata` and the function `const fn header() -> PacketHeader` for the given struct.
/// You must also pass additional attributes with `#[packet]`, specifically packet ID and optionally, encryption and whether to use TCP or UDP (only applicable when sending).
/// Example:
/// ```rust
/// #[derive(Packet, Encodable, Decodable)]
/// #[packet(id = 10000, encrypted = false, tcp = false)] // 'encrypted' and 'tcp' are optional and off by deafult
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
    let enc = opts.encrypted.unwrap_or(false);
    let tcp = opts.tcp.unwrap_or(false);
    let (impl_generics, ty_generics, where_clause) = input.generics.split_for_impl();

    let output = match &input.data {
        Data::Struct(_) => {
            quote! {
                impl #impl_generics PacketMetadata for #ident #ty_generics #where_clause {
                    const PACKET_ID: u16 = #id;
                    const ENCRYPTED: bool = #enc;
                    const SHOULD_USE_TCP: bool = #tcp;
                    const NAME: &'static str = stringify!(#ident);
                }

                impl #impl_generics Packet for #ident #ty_generics #where_clause {}

                impl #impl_generics #ident #ty_generics #where_clause {
                    #[inline]
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

/// Get the repr type of an enum as a token
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

    repr_type.unwrap_or_else(|| {
        Span::call_site().warning("enum repr type not specified - assuming i32. it is recommended to add #[repr(type)] before the enum as that makes it more explicit.").emit();
        quote! { i32 }
    })
}
