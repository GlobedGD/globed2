# esp

`esp` is a lightweight binary serialization library. It provides derive macros for `Encodable`, `Decodable`, `StaticSize`, and `DynamicSize` found in the `derive` crate, not here.

The name stands for **e**fficient **s**erialization **p**rotocol, and is also a play on the x86 stack pointer register, as the crate includes types like `FastByteBuffer` and `FastString`, which make heapless encoding easier.

## Encodable and Decodable

`esp` enables encoding structs to binary data and decoding them back. While this may sound simple, one special feature of the library is its support for **bitfield structs**. Bitfields can be defined as follows:

```rust
#[derive(Encodable, Decodable, StaticSize)]
#[bitfield(on = true, size = 2)] // size is optional and in bytes
pub struct Flags {
    pub b1: bool,
    pub b2: bool,
    pub b3: bool,
    pub b4: bool,
}

assert!(Flags::ENCODED_SIZE == 2); // without `size = 2` would've been 1 byte, without `bitfield(...)` would've been 4 bytes
```

## StaticSize and DynamicSize

### `StaticSize`
`StaticSize` is a trait with a single constant integer, which introduces zero runtime overhead, and all usages can be inlined. However, it is imprecise, as it only indicates the *maximum* size allowed. Therefore, the bytes written by `buffer.write_value(val: &T) where T: StaticSize` will be less than or equal to `T::ENCODED_SIZE`.

### `DynamicSize`
`DynamicSize` is a trait with a method used to calculate the precise encoded size. If the object hasn't been mutated, `val.encoded_size()` will return the exact amount of bytes written by `buffer.write_value(&val)`. However, calculating this involves runtime overhead, as the method must check the size of every struct member to calculate the precise encoded data size.

### Implementing StaticSize and DynamicSize
For efficiency, it’s recommended that every type (except for packets) implementing `StaticSize` should also implement `DynamicSize`. The crate offers the `dynamic_size_as_static_impl!` macro to automatically implement `DynamicSize` for a type that implements `StaticSize`, which will return the static size instead of calculating it.

#### Implementation Guide:
- If the size is known to be a constant N, implement both `StaticSize` and `DynamicSize` to return N.
- If the size varies from 0 to N and memory overhead is minimal, implement `StaticSize` as N and `DynamicSize` to return the exact size.
- If the size is unpredictable (e.g., `Vec` with unlimited elements), only implement `DynamicSize`.

**Important**: You should not use `StaticSize`/`ENCODED_SIZE` when implementing `DynamicSize` (except for types with a constant size like primitive integers). For example, avoid implementing `DynamicSize` for `Vec<T: StaticSize>` using the formula `self.len() * T::ENCODED_SIZE`, as it will lead to incorrect results in certain cases (e.g., `Vec<Option<i32>>`).

## Ready Implementations

`esp` provides implementations for `Encodable`, `Decodable`, and optionally `StaticSize` and/or `DynamicSize` for the following types:

- Primitive integers: `bool`, `u8`, `u16`, `u32`, `u64`, `i8`, `i16`, `i32`, `i64`, `f32`, `f64`
- `str`, `String`
- `Option<T>`, `Result<T, E>`
- Arrays and slices: `[T; N]`, `[T]`
- `Vec<T>`
- `HashMap<K, V, S>`
- `Ipv4Addr`, `SocketAddrV4`
- Tuples: `(T1, T2)`

Variable-size types like `String`, `Vec`, and `HashMap` are prefixed with a `esp::VarLength` (currently an alias for `u16`), indicating the length. This means they can store no more than 65535 elements.

## New Types

`esp` introduces several types optimized for encoding/decoding, but they can also serve other purposes:

### FastString
`FastString` is a Small String Optimization (SSO) class that can store up to 63 characters inline, resorting to heap allocation only when larger storage is required. It can be easily converted to and from a `String` or `&str`.

When encoded, `FastString` behaves identically to `String` or `&str`, but its main advantage is avoiding heap allocation when the string is small.

### InlineString<N>
`InlineString<N>` is a simpler string type that stores all the data inline, never using heap allocation. However, if the string exceeds the defined size (`N`), the program will panic. Use `extend_safe` or manually check the size and capacity before mutating the string to avoid panics.

### FastVec
`FastVec<T, N>` is similar to `InlineString`, but it is a vector-like type that can hold up to `N` elements without allocating on the heap. Adding more than `N` elements causes a panic. It also includes safe APIs like `safe_` that return an error on failure.

### Either
`Either` is a type similar to `Result`, but it conveys the idea of two possible successful outcomes instead of one error and one success.

### FiniteF32, FiniteF64
These are simple wrappers around `f32` and `f64` that return a `DecodeError` if the value is `NaN` or `±inf` during decoding.

### Bits
`Bits<N>` holds a bitfield of flags. For example, `Bits<2>` and `u16` occupy the same size, but the former ensures that the flags are never byte-swapped on non-big-endian systems and offers more user-friendly APIs for managing bits.

### Vec1L
A wrapper around `Vec` that can hold up to 255 elements, with the length encoded as a single byte.

### RemainderBytes
This type must be placed at the end of a struct and will simply store any remaining data in the buffer.

## Additional Notes

The library extends `ByteBuffer` and `ByteReader` from the `bytebuffer` crate with the `ByteBufferExt`, `ByteBufferExtRead`, and `ByteBufferExtWrite` traits. These extensions provide useful methods, though the derive macros often eliminate the need to manually use them.

A key addition is the `ByteBufferExtWrite::append_self_checksum` and `ByteBufferExtRead::validate_self_checksum` methods. The first calculates an Adler-32 checksum of the buffer’s data, encodes it as a 32-bit unsigned integer, and writes it. The latter method validates the checksum during decoding and raises a `DecodeError` if the data doesn’t match the checksum.
