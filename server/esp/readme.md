# esp

binary serialization protocol library. derive macros for `Encodable`, `Decodable` and `KnownSize` will be found in the `derive` crate, not here.

name meaning - **e**fficient **s**erialization **p**rotocol + a play on the stack pointer register on x86 since the crate also adds things like `FastByteBuffer` and `FastString` making heapless encoding easier.