use serde::Serialize;

#[allow(dead_code)]
#[derive(Debug)]
pub enum ProtocolError {
    TooShort(usize),
    SerializeError(rmp_serde::encode::Error),
    DeserializeError(rmp_serde::decode::Error),
}

impl std::fmt::Display for ProtocolError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::TooShort(len) => write!(f, "message too short: {len} bytes"),
            Self::SerializeError(e) => write!(f, "serialize: {e}"),
            Self::DeserializeError(e) => write!(f, "deserialize: {e}"),
        }
    }
}

impl std::error::Error for ProtocolError {}

/// Encode a message: 1-byte type ID + MessagePack body (named/map keys for wire compat).
pub fn encode_message<T: Serialize>(type_id: u8, payload: &T) -> Result<Vec<u8>, ProtocolError> {
    let body = rmp_serde::to_vec_named(payload).map_err(ProtocolError::SerializeError)?;
    let mut frame = Vec::with_capacity(1 + body.len());
    frame.push(type_id);
    frame.extend_from_slice(&body);
    Ok(frame)
}

/// Decode a message: returns (type_id, payload_bytes).
/// Caller deserializes payload_bytes with the appropriate struct.
pub fn decode_message(data: &[u8]) -> Result<(u8, &[u8]), ProtocolError> {
    if data.len() < 2 {
        return Err(ProtocolError::TooShort(data.len()));
    }
    Ok((data[0], &data[1..]))
}
