const functions = require("firebase-functions"),
admin = require("firebase-admin");

const app = admin.initializeApp();
const firestore = app.firestore();
var num = 0;

firestore.settings({ timestampsInSnapshots: true });

const auth = app.auth();


exports.storeBikecounterData = (req, res) => {
    let payload = req.body;
    let deviceId;
    const app_id = payload.end_device_ids.application_ids.application_id;
    if (payload && app_id == "bikecounter") {
        deviceId = payload.end_device_ids.device_id;
        const devicePayload = payload.uplink_message.decoded_payload;
        const batteryLevel = devicePayload.batteryLevel;
        const batteryVoltage = devicePayload.batteryVoltage;
        const humidity = devicePayload.humidity;
        const temperature = devicePayload.temperature;
        const stat = devicePayload.stat;
        const swVersion = devicePayload.swVersion;
        const hwVersion = devicePayload.hwVersion;
        const timeArray = devicePayload.timeArray;
        const gateways = payload.uplink_message.rx_metadata[0].gateway_ids;
        const count = devicePayload.count;
        let transmissionTime = devicePayload.deviceTransmissionTime;
        if (transmissionTime){
            //is sent as seconds since 1970 UTC
            transmissionTime*1000;
        }

        //create a map with unique timestamps as keys and sum up the counts per timestamp
        let map = new Map();
        timeArray.forEach(t => {
            if (!map.get(t)){
                map.set(t, 1);
            } else {
                map.set(t, map.get(t)+1)
            }
        });

        try {
            firestore.collection(`${deviceId}`).add({'counter': 0, 'timestamp': new Date(transmissionTime).toISOString(), 'batteryLevel': batteryLevel, 'batteryVoltage': batteryVoltage, 'humidity': humidity, 'temperature': temperature, 'stat': stat, 'gateways': gateways, 'swVersion': swVersion, 'hwVersion': hwVersion});
            console.log(`Added data for ${deviceId}`);
            
            //one more DB entry for every timestamp
            for (let timestamp of map.keys()) {
                let date = new Date(timestamp).toISOString();
                firestore.collection(`${deviceId}`).add({'counter': map.get(timestamp), 'timestamp': date });
                console.log(`Added data for ${deviceId}`);
            }
        } catch (error) {
            console.error(`error while trying to store data for: ${deviceId}`, error);
        }
        res.status(200).send(deviceId);
    } else {
        console.error("payload not valid: "+JSON.stringify(payload));
        res.status(404).send(deviceId);
    }
};