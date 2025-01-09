use crate::*;

const fn constmax(a: usize, b: usize) -> usize {
    if a >= b { a } else { b }
}

#[derive(Clone, Debug)]
pub enum Either<T, Y> {
    First(T),
    Second(Y),
}

impl<T, Y> Either<T, Y> {
    pub const fn new_first(val: T) -> Self {
        Self::First(val)
    }

    pub const fn new_second(val: Y) -> Self {
        Self::Second(val)
    }

    pub const fn is_first(&self) -> bool {
        match self {
            Self::First(_) => true,
            Self::Second(_) => false,
        }
    }

    pub const fn is_second(&self) -> bool {
        !self.is_first()
    }

    pub fn unwrap_first(self) -> T {
        match self {
            Self::First(x) => x,
            Self::Second(_) => panic!("attempting to call unwrap_first an Either<T, Y> containing Y"),
        }
    }

    pub fn unwrap_second(self) -> Y {
        match self {
            Self::First(_) => panic!("attempting to call unwrap_second an Either<T, Y> containing T"),
            Self::Second(y) => y,
        }
    }

    pub fn first(self) -> Option<T> {
        match self {
            Self::First(x) => Some(x),
            Self::Second(_) => None,
        }
    }

    pub fn second(self) -> Option<Y> {
        match self {
            Self::Second(y) => Some(y),
            Self::First(_) => None,
        }
    }

    pub const fn as_ref(&self) -> Either<&T, &Y> {
        match *self {
            Self::First(ref x) => Either::First(x),
            Self::Second(ref y) => Either::Second(y),
        }
    }
}

impl<T, Y> Encodable for Either<T, Y>
where
    T: Encodable,
    Y: Encodable,
{
    #[inline]
    fn encode(&self, buf: &mut ByteBuffer) {
        buf.write_bool(self.is_first());
        match self {
            Self::First(x) => buf.write_value(x),
            Self::Second(y) => buf.write_value(y),
        }
    }

    #[inline]
    fn encode_fast(&self, buf: &mut FastByteBuffer) {
        buf.write_bool(self.is_first());
        match self {
            Self::First(x) => buf.write_value(x),
            Self::Second(y) => buf.write_value(y),
        }
    }
}

impl<T, Y> Decodable for Either<T, Y>
where
    T: Decodable,
    Y: Decodable,
{
    #[inline]
    fn decode(buf: &mut ByteBuffer) -> DecodeResult<Self>
    where
        Self: Sized,
    {
        Ok(if buf.read_bool()? {
            Self::First(buf.read_value::<T>()?)
        } else {
            Self::Second(buf.read_value::<Y>()?)
        })
    }

    #[inline]
    fn decode_from_reader(buf: &mut ByteReader) -> DecodeResult<Self>
    where
        Self: Sized,
    {
        Ok(if buf.read_bool()? {
            Self::First(buf.read_value::<T>()?)
        } else {
            Self::Second(buf.read_value::<Y>()?)
        })
    }
}

// not sure if this one makes any sense
impl<T, Y> StaticSize for Either<T, Y>
where
    T: StaticSize,
    Y: StaticSize,
{
    const ENCODED_SIZE: usize = size_of_types!(bool) + constmax(T::ENCODED_SIZE, Y::ENCODED_SIZE);
}

impl<T, Y> DynamicSize for Either<T, Y>
where
    T: DynamicSize,
    Y: DynamicSize,
{
    fn encoded_size(&self) -> usize {
        size_of_types!(bool)
            + match self {
                Self::First(x) => x.encoded_size(),
                Self::Second(y) => y.encoded_size(),
            }
    }
}
