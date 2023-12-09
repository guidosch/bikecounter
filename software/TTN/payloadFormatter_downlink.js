function encodeDownlink(input) {
  var seconds = input.data.timeDrift >> 0;
  var encodedSeconds = [0, 0, 0, 0];
  encodedSeconds[0] = seconds & 0xff;
  encodedSeconds[1] = (seconds >> 8) & 0xff;
  encodedSeconds[2] = (seconds >> 16) & 0xff;
  encodedSeconds[3] = (seconds >> 24) & 0xff;
  return {
    bytes: encodedSeconds,
    fPort: 1,
    warnings: [],
    errors: [],
  };
}

function decodeDownlink(input) {
  var td = input.bytes[3];
  td = (td << 8) | input.bytes[2];
  td = (td << 8) | input.bytes[1];
  td = (td << 8) | input.bytes[0];
  return {
    data: {
      bytes: input.bytes,
      timeDrift: td,
    },
    warnings: [],
    errors: [],
  };
}
