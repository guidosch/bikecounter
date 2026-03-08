const functions = require('@google-cloud/functions-framework');
admin = require("firebase-admin");
const https = require("https");
const crypto = require("crypto");

const app = admin.initializeApp();
const firestore = app.firestore();
const db = admin.firestore();

firestore.settings({ timestampsInSnapshots: true });

functions.http('processDataSwisscom', (req, res) => {
    
    const hexString = req.body.DevEUI_uplink.payload_hex;
    const byteArray = Uint8Array.from(hexString.match(/.{1,2}/g).map((byte) => parseInt(byte, 16)));
    let devicePayload = decodeUplink({ bytes: byteArray }).data;
    processData(req.body.DevEUI_uplink, devicePayload, res);
})


/**
 * Process our custom payload and some metadata and store to firebase
 */
function processData(payload, devicePayload) {
    const app_id = payload.CustomerData.tags[0]; //todo find correct tag
    const deviceId = payload.DevEUI;
    const deviceEUI = payload.DevEUI;
    if (devicePayload) {
        const timeArray = devicePayload.timeArray;
        const timeDrift = devicePayload.timeDrift;
        let transmissionTime = devicePayload.deviceTransmissionTime;
        if (transmissionTime) {
            //is sent as seconds since 1970 UTC
            transmissionTime = transmissionTime * 1000;
        } else {
            transmissionTime = new Date().getTime();
        }

        //create a map with unique timestamps as keys and sum up the counts (one timestamp per trigger) per timestamp
        let map = new Map();
        timeArray.forEach((t) => {
            if (!map.get(t)) {
                map.set(t, 1);
            } else {
                map.set(t, map.get(t) + 1);
            }
        });

        // LoRa network informations
        let gateways = [];
        for (let gateway of payload.Lrrs.Lrr) {
            gateways.push({
                id: gateway.Lrrid || "not set",
                eui: gateway.Lrrid || "not set",
                rssi: gateway.LrrRSSI || "not set",
                snr: gateway.LrrSNR || "not set",
            });
        }
        const airtime = 0;

        // check the time deviation and post the sync downlink if necessary
        processTimeSync(payload, timeDrift);

        switch (app_id) {
            case "bikecounter":
                // statId == 7 is the time sync call
                if (devicePayload.statId != 7) {
                    // get the collection id (trail) from the deviceEUI
                    db.collection("internal-deviceId-trail-ct")
                        .where("deviceEUI", "==", deviceEUI)
                        .orderBy("validFrom", "desc")
                        .limit(1)
                        .get()
                        .then((snapshot) => {
                            if (snapshot.empty) {
                                console.warn("No trail for device found! DeviceEUI=", deviceEUI);
                                res.status(404).send(deviceId);
                            } else {
                                snapshot.forEach((doc) => {
                                    const collId = doc.data().collectionID;
                                    console.log(
                                        "Device association found: deviceEUI=",
                                        deviceEUI,
                                        " trail/collection=",
                                        collId
                                    );

                                    // Build data document
                                    const dataDoc = devicePayload;
                                    dataDoc.counter = 0;
                                    dataDoc.timestamp = new Date(transmissionTime).toISOString();
                                    dataDoc.gateways = gateways;
                                    dataDoc.airtime = airtime;
                                    dataDoc.deviceEUI = deviceEUI;
                                    dataDoc.deviceId = deviceId;

                                    // store the parsed payload into the trail collection
                                    firestore.collection(`${collId}`).add(dataDoc);
                                    console.log(`Added health data for ${collId}`);

                                    //one more DB entry for every timestamp
                                    for (let timestamp of map.keys()) {
                                        let date = new Date(timestamp).toISOString();
                                        firestore
                                            .collection(`${collId}`)
                                            .add({ counter: map.get(timestamp), timestamp: date });
                                        console.log(`Added data for ${collId}`);
                                    }

                                    res.status(200).send(deviceId);
                                });
                            }
                        })
                        .catch((err) => {
                            console.error(
                                "Error while trying to store data for device: ",
                                deviceEUI,
                                " error-msg:",
                                err
                            );
                            res.status(500).send(deviceId);
                        });
                } else {
                    console.log("TimeSync request");
                    res.status(200).send(deviceId);
                }
                break;

            case "bikecounter-dev":
                console.log("Dev-App request received. (no database update)");
                res.status(200).send(deviceId);
                break;

            default:
                console.warn("Request does not match the criteria");
                res.status(401).send(deviceId);
                break;
        }
    } else {
        //console.error("payload not valid: " + JSON.stringify(payload));
        console.error("Payload not valid");
        res.status(400).send(deviceId);
    }
};

/**
 * send the timedrif correction as LORA downlink to the device
 */
function processTimeSync(timeDrift) {
    // send downlink package with timeDrift information
    if (Math.abs(timeDrift) > 15 * 60) {
        // create package data
        const data = JSON.stringify({
            downlinks: [
                {
                    decoded_payload: {
                        timeDrift: timeDrift,
                    },
                },
            ],
        });

        // POST request options
        const TIAK = "9a8e0d4050114bad87019d547477553a"; //read from ENV
        const AS_ID = "TWA_100055533.77834.AS";
        const fPort = "1";
        const nowIso = new Date().toISOString();

        const queryParams = {
            AS_ID: AS_ID,
            DevEUI: payload.DevEUI,
            FPort: fPort,
            Time: nowIso.replace(/\.\d+Z$/, 'Z')
        };
        const sortedKeys = Object.keys(queryParams).sort();
        const queryStringForSig = sortedKeys.map(k => `${k}=${queryParams[k]}`).join('&');

        // Build the signature input (body + '&' + query + '&' + key) —
        // exact separators may differ in your tenant’s doc; this is a clear, consistent approach.
        const sigInput = `${data}&${queryStringForSig}&${TIAK}`;
        const token = crypto.createHash('sha256').update(sigInput, 'utf8').digest('hex');

        const options = {
            hostname: "portal.lpn.swisscom.ch",
            port: 443,
            path: `/thingpark/lrc/rest/v2/downlink?${queryStringForSig}&Token=${token}`,
            method: "POST",
            headers: {
                "Content-Type": "application/json",
                "Content-Length": data.length,
            },
        };

        //console.log("options: "+JSON.stringify(options));

        // perform the post request to the thinkpark webhook
        const reqDown = https.request(options, (resDown) => {
            console.log(`statusCode: ${res.statusCode}`);

            resDown.on("data", (d) => {
                process.stdout.write(d);
            });
        });

        reqDown.on("error", (error) => {
            console.error(error);
        });

        reqDown.write(data);
        reqDown.end();
    }
}

/**
 * Decode our custom lora payload
 */
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
    let coef = [
        -35946.107099583, 52310.9370900473, -29962.8071041224, 8431.4105127835,
        -1164.3507315616, 63.1475757459,
    ];
    let b = data.batteryVoltage;
    data.batteryLevel = Math.round(
        coef[5] * b * b * b * b * b +
        coef[4] * b * b * b * b +
        coef[3] * b * b * b +
        coef[2] * b * b +
        coef[1] * b +
        coef[0]
    );
    // temp. fix for batteryLevel undefined
    if (!data.batteryLevel) {
        data.batteryLevel = 0;
    }
    // temperature
    let tempIndex = input.bytes[3] & 0x1f;
    data.temperature = Math.round(((70 / (32 - 1)) * tempIndex - 20) * 10) / 10;
    // humidity
    let humIndex = input.bytes[3] >> 5;
    data.humidity = Math.round((100 / (8 - 1)) * humIndex * 10) / 10;
    // timer interval
    data.intervalId = input.bytes[4] & 0x07;
    var intervalTime = {
        0: 1,
        1: 2,
        2: 4,
        3: 8,
        4: 17,
    };
    var intervalBitSize = [6, 7, 8, 9, 10];
    data.selectedInterval = "< " + intervalTime[data.intervalId] + "h";
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
    // correct the date if it is the first call of the day
    if (ts.getUTCHours === 0) {
        ts.setUTCHours(ts.getUTCHours() - 1);
    }
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
    console.log("Decoded payload: ", JSON.stringify(data));

    return {
        data: data,
        warnings: [],
        errors: [],
    };
}

/**
 * Encode the time drift into a downlink payload
 * @param {*} input 
 * @returns 
 */
function encodeDownlink(timeDrift) {
    var seconds = timeDrift >> 0;
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


