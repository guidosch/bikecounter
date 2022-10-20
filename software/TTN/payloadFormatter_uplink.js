function decodeUplink(input) {
  var data = {};
  // count
  data.count = input.bytes[0];
  // status
  var statusCode = {
    0: "no error",
    1: "1",
    2: "2",
    3: "3",
    4: "4",
    5: "5",
    6: "6",
    7: "sync call",
  };

  data.swVersion = input.bytes[1] & 0x0f;
  data.hwVersion = input.bytes[1] >> 4;
  data.statId = input.bytes[2] & 0x07;
  data.stat = statusCode[data.statId];
  // battery level
  let batteryIndex = input.bytes[2] >> 3;
  data.batteryVoltage =
    Math.round(((1.5 / (32 - 1)) * batteryIndex + 3) * 100) / 100;
  let coef = [-35946.107099583, 52310.9370900473, -29962.8071041224, 8431.4105127835, -1164.3507315616, 63.1475757459];
  let b = data.batteryVoltage;
  data.batteryLevel = coef[5]*b*b*b*b*b + coef[4]*b*b*b*b + coef[3]*b*b*b + coef[2]*b*b + coef[1]*b + coef[0];
  // temperature
  let tempIndex = input.bytes[3] & 0x1f;
  data.temperature = Math.round(((70 / (32 - 1)) * tempIndex - 20) * 10) / 10;
  // humidity
  let humIndex = input.bytes[3] >> 5;
  data.humidity = Math.round((100 / (8 - 1)) * humIndex * 10) / 10;
  // timer interval
  data.intervalId = input.bytes[4] & 0x07;
  var intervalTime = {
    0: "< 1h",
    1: "< 2h",
    2: "< 4h",
    3: "< 8h",
    4: "< 17h",
  };
  var intervalBitSize = [6, 7, 8, 9, 10];
  data.selectedInterval = intervalTime[data.intervalId];
  // start hour of day
  data.hourOfDay = input.bytes[4] >> 3;
  if (data.swVersion > 0) {
    // device time (epoch)
    data.deviceTime = input.bytes[7] >> 0;
    data.deviceTime = (data.deviceTime << 8) | input.bytes[6];
    data.deviceTime = (data.deviceTime << 8) | input.bytes[5];
    data.deviceTime *= 60; // device sends the epoch time in minutes
    data.deviceTime += 1640995200; //start offset 01.01.2022
    //calculate time drift in seconds
    var today = new Date();
    var serverEpoch = (today.getTime() / 1000) >> 0; // seconds since 1 Jan 1970
    data.timeDrift = serverEpoch - data.deviceTime;
  } else {
    data.timeDrift = 0;
  }

  // decode payload time array
  var offsetBits;
  if (data.swVersion > 0) {
    offsetBits = 8 * 8;
  } else {
    offsetBits = 5 * 8;
  }
  var buffer = new ArrayBuffer(data.count);
  var absMinArray = new Int8Array(buffer);
  for (var j = 0; j < data.count; j++) {
    absMinArray[j] = 0;
  }

  for (
    var payloadBit = offsetBits;
    payloadBit < data.count * intervalBitSize[data.intervalId] + offsetBits;
    payloadBit++
  ) {
    var currentMotionByte = Math.floor(
      (payloadBit - offsetBits) / intervalBitSize[data.intervalId]
    );
    var currentMotionBit = Math.floor(
      (payloadBit - offsetBits) % intervalBitSize[data.intervalId]
    );
    var currentMotionBitMask = 1 << currentMotionBit;
    var currentPayloadByte = Math.floor(payloadBit / 8);
    var currentPayloadBit = Math.floor(payloadBit % 8);
    var currentPayloadBitMask = 1 << currentPayloadBit;

    var readBit = input.bytes[currentPayloadByte] & currentPayloadBitMask;

    if (readBit !== 0) {
      // set bit
      absMinArray[currentMotionByte] |= currentMotionBitMask;
    }
  }
  var hourArray = [];
  var minArray = [];
  for (var k = 0; k < data.count; k++) {
    hourArray.push(data.hourOfDay + Math.floor(absMinArray[k] / 60));
    minArray.push(absMinArray[k] % 60);
  }
  // create output time array
  var ts = new Date(Date.now());
  ts.setUTCHours(0);
  ts.setUTCMinutes(0);
  ts.setUTCSeconds(0);
  ts.setUTCMilliseconds(0);

  data.timeArray = [];
  for (var l = 0; l < data.count; l++) {
    var ts_i = new Date(ts);
    ts_i.setUTCHours(hourArray[l]);
    ts_i.setUTCMinutes(minArray[l]);
    data.timeArray.push(ts_i.getTime());
  }

  return {
    data: data,
    warnings: [],
    errors: [],
  };
}
