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
    let batteryIndex = input.bytes[1] >> 3;
    data.batteryLevel = 100 / (32 - 1) * batteryIndex;
    // temperatur
    let tempIndex = input.bytes[2] & 0x1F;
    data.temperatur = 70 / (32 - 1) * tempIndex - 20;
    // humidity
    let humIndex = input.bytes[2] >> 5;
    data.humidity = 100 / (8 - 1) * humIndex;
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

    for (var payloadBit = offsetBits; payloadBit < ((data.count * intervalBitSize[data.intervallId]) + offsetBits); payloadBit++) {
        var currentMotionByte = Math.floor((payloadBit - offsetBits) / intervalBitSize[data.intervallId]);
        var currentMotionBit = Math.floor((payloadBit - offsetBits) % intervalBitSize[data.intervallId]);
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
        errors: []
    };
}

/*
// Motion detected (current count = 1 / time: 19:56:57)
// Motion detected (current count = 2 / time: 19:57:9)
// Motion detected (current count = 3 / time: 19:58:13)
// Motion detected (current count = 4 / time: 19:58:17)
// Motion detected (current count = 5 / time: 19:58:20)
// Motion detected (current count = 6 / time: 19:58:24)
// Motion detected (current count = 7 / time: 19:58:27)
// Motion detected (current count = 8 / time: 19:58:31)
// Motion detected (current count = 9 / time: 19:58:34)
// Motion detected (current count = 10 / time: 19:58:37)
// Message sent correctly! (count = 10 / temperature = 22.90°C / humidity = 42.86% / battery level = 25.81 % / 2.96 V)
*/
var input = {};
input.bytes = [0x0A, 0x40, 0x73, 0x98, 0x78, 0xAE, 0xEB, 0xBA, 0xAE, 0xEB, 0xBA, 0x0E];
//input.bytes = [0x0A, 0x40, 0x73, 0xA8, 0x59, 0x96, 0x65, 0x9A, 0xA6, 0x69, 0x9A, 0x06];
var test = decodeUplink(input);

console.log(test.data.timeArray);

