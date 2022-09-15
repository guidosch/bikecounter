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
  return {
    data: {
      bytes: input.bytes,
    },
    warnings: [],
    errors: [],
  };
}
