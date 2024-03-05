# esp

lightweight binary serialization library. derive macros for `Encodable`, `Decodable`, `StaticSize` and `DynamicSize` will be found in the `derive` crate, not here.

name meaning - **e**fficient **s**erialization **p**rotocol + a play on the stack pointer register on x86 since the crate also provides types like `FastByteBuffer` and `FastString` making heapless encoding easier.

## Encodable and Decodable

it's just encoding structs to binary data. or the other way around. do i really need to say more

## StaticSize and DynamicSize

Short primer on those traits -

`StaticSize` is a trait with a single constant integer, so there is zero runtime overhead and any usages can be inlined. However it is imprecise and only indicates the *maximum* permitted size, which means that after calling `buffer.write_value(val: &T) where T: StaticSize`, the amount of bytes written is equal to *or less than* `T::ENCODED_SIZE`.

`DynamicSize` is a trait with a non-static method, which is used to calculate the precise encoded size. If the object hasn't been mutated between the two calls, there is a guarantee that the result of `val.encoded_size()` will be exactly the amount of bytes written with `buffer.write_value(&val)`. This precision, however, comes with a downside of a runtime calculation, as this will have to go through every struct member and check their sizes in order to calculate the exact size of the encoded data.

It is encouraged that every type (except for packets) that implements `StaticSize` must also implement `DynamicSize`, or else it would not be possible to derive `DynamicSize` for a struct containing such types. For efficiency purposes, the crate provides `dynamic_size_as_static_impl!` macro which would automatically implement `DynamicSize` for a struct that implements `StaticSize`, and that implementation would just return the static size instead of doing any calculations. That is also available as a derive macro atribute `#[dynamic_size(as_static = true)]`

Implementer guide (only needed if you can't or don't want to derive):

* If the size is known to be a constant N, implement both traits to return N.
* If the size is known to be from 0 to N (inclusive), and N is small enough that memory allocation overhead doesn't matter, implement `StaticSize` to be N, and implement `DynamicSize` to calculate and return the exact encoded size.
* If the size is too unpredictable (for example `Vec` which has unlimited size), only implement `DynamicSize`.

You should **not** use `StaticSize`/`ENCODED_SIZE` *at all* when implementing `DynamicSize`, except for types that have an *actual constant size* (like primitive integers). For example, do not implement `DynamicSize` for `Vec<T: StaticSize>` with the implementation `self.len() * T::ENCODED_SIZE`. This is a bad idea, and while it may work properly for something like `Vec<i32>`, it will produce incorrect results for something like `Vec<Option<i32>>`. The entire point of `DynamicSize` is to be *accurate*, whereas `StaticSize` is made to be *fast*.

## Ready implementations

`esp` provides the implementations of `Encodable`, `Decodable`, and optionally `StaticSize` and/or `DynamicSize` depending on the type, for all the following types:

* Primitive integers (`bool`, `u8`, `u16`, `u32`, `u64`, `i8`, `i16`, `i32`, `i64`, `f32`, `f64`)
* `str`
* `String`
* `Option<T>`
* `Result<T, E>`
* `[T; N]`
* `[T]`
* `Vec<T>`
* `HashMap<K, V>`
* `Ipv4Addr`
* `SocketAddrV4`
* `(T1, T2)`

## Added types

`esp` also adds a few new types, which are optimized specifically for the purpose of encoding or decoding, but can also be used for other purposes.

### FastString

`FastString<N>` is a simple string class that stores all of the data in itself - no allocation. It can be easily converted to and from a `String` or `&str` if needed.

When encoded, the actual representation is no different from `String` or `&str`, so its main purpose is just avoiding heap allocation.

Do keep in mind that if you try to mutate the string and the size goes above `N`, the program will panic. To avoid that, use `extend_safe` or manually check the size and capacity of the string before mutating.

### FastVec

Similar to `FastString`, `FastVec<T, N>` is a class with similar API to `Vec<T>`, except it uses no heap storage and can store up to `N` elements, at which point adding any more will cause the program to panic. Just like `FastString`, it also has `safe_` APIs that instead return an error on failure.

### FiniteF32, FiniteF64

Simple wrappers around `f32` and `f64` with the only difference being that they will return a `DecodeError` if the value is `nan` or `inf` when decoding.

### RemainderBytes

Must be put at the end of a struct, the value will simply be the remaining data in the buffer.

## Additional notes

The library wraps around `ByteBuffer` and `ByteReader` from the crate `bytebuffer` and extends them both with traits `ByteBufferExt`, `ByteBufferExtRead`, `ByteBufferExtWrite`. These have a lot of useful methods, but due to derive macros, you often don't have to manually use those.

One notable addition is methods `ByteBufferExtWrite::append_self_checksum` and `ByteBufferExtRead::validate_self_checksum`. The former calculates an Adler-32 checksum of all the data in the buffer, encodes it as a 32-bit unsigned integer, and writes it at the current position of the buffer. The latter method decodes that checksum and verifies that the data in the buffer matches, otherwise raising a `DecodeError`.