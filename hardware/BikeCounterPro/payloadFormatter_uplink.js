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
        6: "6"
    };
    data.stat = statusCode[(input.bytes[1] & 0x07)];
    // battery level
    data.batteryIndex = input.bytes[1] >> 3;
    data.batteryLevel = 100 / (32 - 1) * data.batteryIndex;
    // temperatur
    data.tempIndex = input.bytes[2] & 0x1F;
    data.temperatur = 70 / (32 - 1) * data.tempIndex - 20;
    // humidity
    data.humIndex = input.bytes[2] >> 5;
    data.humidity = 100 / (8 - 1) * data.humIndex;
    // timer interval
    data.intervallId = input.bytes[3] & 0x07;
    var intervalTime = {
        0: "< 1h",
        1: "< 2h",
        2: "< 4h",
        3: "< 8h",
        4: "< 17h"
    };
    var intervalBitSize = [6, 7, 8, 9, 10];
    data.selectedIntervall = intervalTime[data.intervallId];
    // start hour of day
    data.hourOfDay = input.bytes[3] >> 3;
    // decode payload time array
    var offsetBits = 4 * 8;
    var buffer = new ArrayBuffer(data.count);
    var absMinArray = new Int8Array(buffer);
    for (var j = 0; j < data.count; j++) {
        absMinArray[j] = 0;
    }
    for (var payloadBit = offsetBits; payloadBit < ((data.count * intervalBitSize[data.selectedIntervall]) + offsetBits); payloadBit++) {
        var currentMotionByte = (payloadBit - offsetBits) / intervalBitSize[data.selectedIntervall];
        var currentMotionBit = (payloadBit - offsetBits) % intervalBitSize[data.selectedIntervall];
        var currentMotionBitMask = (1 << currentMotionBit) >> 1;
        var currentPayloadByte = payloadBit / 8;
        var currentPayloadBit = payloadBit % 8;
        var currentPayloadBitMask = (1 << currentPayloadBit) >> 1;

        var readBit = input.bytes[currentPayloadByte] & currentPayloadBitMask;

        if (readBit === 0) {
            // clear bit
            absMinArray[currentMotionByte] &= ~currentMotionBitMask;
        }
        else {
            // set bit
            absMinArray[currentMotionByte] |= currentMotionBitMask;
        }
    }
    var hourArray = [];
    var minArray = [];
    for (var k = 0; k < data.count; k++) {
        hourArray.push(data.hourOfDay + Math.floor(absMinArray[k] / 60));
        minArray.push(Math.floor(absMinArray[k] % 60));
    }
    // create output time array
    var ts = new Date(Date.now());
    ts.setUTCHours(0);
    ts.setUTCMinutes(0);
    ts.setUTCSeconds(0);
    ts.setUTCMilliseconds(0);
    var ta = [];
    for (var l = 0; l < data.count; l++) {
        var ts_i = ts;
        ts.setUTCHours(hourArray[l]);
        ts_i.setUTCMinutes(minArray[l]);
        ta.push(ts_i);
    }
    // data.date = ts.getUTCFullYear() + "." + (ts.getUTCMonth()+1) + "." + ts.getUTCDate();
    data.timeArray = ta;
    return {
        data: data,
        warnings: [],
        errors: []
    };
}