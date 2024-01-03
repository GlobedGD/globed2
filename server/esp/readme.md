# esp

lightweight binary serialization protocol library. derive macros for `Encodable`, `Decodable`, `StaticSize` and `DynamicSize` will be found in the `derive` crate, not here.

name meaning - **e**fficient **s**erialization **p**rotocol + a play on the stack pointer register on x86 since the crate also provides types like `FastByteBuffer` and `FastString` making heapless encoding easier.

## StaticSize and DynamicSize

Short primer on those traits -

`StaticSize` is a trait with a single constant integer, so there is zero runtime overhead and any usages can be inlined. However it is imprecise and only indicates the *maximum* permitted size, which means that after calling `buffer.write_value(val: &T) where T: StaticSize`, the amount of bytes written is equal to *or less than* `T::ENCODED_SIZE`.

`DynamicSize` is a trait with a non-static method, which is used to calculate the precise encoded size. If the object hasn't been mutated between the two calls, there is a guarantee that the result of `val.encoded_size()` will be exactly the amount of bytes written with `buffer.write_value(&val)`. This precision, however, comes with a downside of a runtime calculation, as this will have to go through every struct member and check their sizes in order to calculate the exact size of the encoded data.

It is encouraged that every type (except for packets) that implements `StaticSize` must also implement `DynamicSize`, or else it would not be possible to derive `DynamicSize` for a struct containing such types. For efficiency purposes, the crate provides `dynamic_size_as_static_impl!` macro which would automatically implement `DynamicSize` for a struct that implements `StaticSize`, and that implementation would just return the static size instead of doing any calculations. That is also available as a derive macro atribute `#[dynamic_size(as_static = true)]`

Implementer guide (only needed if you can't or don't want to derive):

* If the size is known to be a constant N, implement both traits to return N.
* If the size is known to be from 0 to N (inclusive), and N is small enough that memory allocation overhead doesn't matter, implement `StaticSize` to be N, and implement `DynamicSize` to calculate and return the exact encoded size.
* If the size is too unpredictable (for example `Vec` with no upper bound), only implement `DynamicSize`.

You should **not** use `StaticSize`/`ENCODED_SIZE` *at all* when implementing `DynamicSize`, except for types that have an *actual constant size* (like primitive integers). For example, do not derive `DynamicSize` for `Vec<T: StaticSize>` with the implementation `self.len() * T::ENCODED_SIZE`. This is a bad idea, and while it may work properly for something like `Vec<i32>`, it will produce incorrect results for something like `Vec<Option<i32>>`. The entire point of `DynamicSize` is to be *accurate*, whereas `StaticSize` is made to be *fast*.
