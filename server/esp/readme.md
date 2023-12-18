# esp

lightweight binary serialization protocol library. derive macros for `Encodable`, `Decodable`, `StaticSize` and `DynamicSize` will be found in the `derive` crate, not here.

name meaning - **e**fficient **s**erialization **p**rotocol + a play on the stack pointer register on x86 since the crate also provides types like `FastByteBuffer` and `FastString` making heapless encoding easier.

## StaticSize and DynamicSize

Short primer on those traits -

* If the size is known to be a constant N, implement both traits to return N.
* If the size is known to be from 0 to N (inclusive), and N is small enough that memory allocation overhead doesn't matter, implement `StaticSize` to be N, and implement `DynamicSize` to calculate the exact encoded size.
* If the size is too unpredictable (for example `Vec` with no upper bound), only implement `DynamicSize`.

It is encouraged that everything that implements `StaticSize` must implement `DynamicSize`. If without any calculations, at least make it return the same value as `StaticSize`.