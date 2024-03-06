use crate::*;

#[derive(Copy, Clone, Default, Debug)]
pub struct FiniteF32(f32);

impl Encodable for FiniteF32 {
    fn encode(&self, buf: &mut ByteBuffer) {
        buf.write_f32(self.0);
    }

    fn encode_fast(&self, buf: &mut FastByteBuffer) {
        buf.write_f32(self.0);
    }
}

impl Decodable for FiniteF32 {
    fn decode(buf: &mut ByteBuffer) -> DecodeResult<Self>
    where
        Self: Sized,
    {
        let x = buf.read_f32()?;
        if !x.is_finite() {
            return Err(DecodeError::NonFiniteValue);
        }

        Ok(FiniteF32(x))
    }

    fn decode_from_reader(buf: &mut ByteReader) -> DecodeResult<Self>
    where
        Self: Sized,
    {
        let x = buf.read_f32()?;
        if !x.is_finite() {
            return Err(DecodeError::NonFiniteValue);
        }

        Ok(FiniteF32(x))
    }
}

impl StaticSize for FiniteF32 {
    const ENCODED_SIZE: usize = size_of_types!(f32);
}

impl DynamicSize for FiniteF32 {
    fn encoded_size(&self) -> usize {
        Self::ENCODED_SIZE
    }
}

impl Display for FiniteF32 {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}", self.0)
    }
}

#[derive(Copy, Clone, Default, Debug)]
pub struct FiniteF64(f64);

impl Encodable for FiniteF64 {
    fn encode(&self, buf: &mut ByteBuffer) {
        buf.write_f64(self.0);
    }

    fn encode_fast(&self, buf: &mut FastByteBuffer) {
        buf.write_f64(self.0);
    }
}

impl Decodable for FiniteF64 {
    fn decode(buf: &mut ByteBuffer) -> DecodeResult<Self>
    where
        Self: Sized,
    {
        let x = buf.read_f64()?;
        if !x.is_finite() {
            return Err(DecodeError::NonFiniteValue);
        }

        Ok(FiniteF64(x))
    }

    fn decode_from_reader(buf: &mut ByteReader) -> DecodeResult<Self>
    where
        Self: Sized,
    {
        let x = buf.read_f64()?;
        if !x.is_finite() {
            return Err(DecodeError::NonFiniteValue);
        }

        Ok(FiniteF64(x))
    }
}

impl StaticSize for FiniteF64 {
    const ENCODED_SIZE: usize = size_of_types!(f64);
}

impl DynamicSize for FiniteF64 {
    fn encoded_size(&self) -> usize {
        Self::ENCODED_SIZE
    }
}

impl Display for FiniteF64 {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}", self.0)
    }
}
