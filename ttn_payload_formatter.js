function decodeUplink(input) {
  const bytes = input.bytes;

  if (!Array.isArray(bytes) || bytes.length !== 2) {
    return {
      errors: ["Expected exactly 2 payload bytes for temperature."]
    };
  }

  const unsignedValue = (bytes[0] << 8) | bytes[1];
  const signedValue = unsignedValue & 0x8000
    ? unsignedValue - 0x10000
    : unsignedValue;

  return {
    data: {
      temperature_c: signedValue / 100
    }
  };
}
