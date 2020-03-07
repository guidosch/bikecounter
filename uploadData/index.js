const functions = require("firebase-functions"),
    admin = require("firebase-admin");

const app = admin.initializeApp();
const firestore = app.firestore();
var num = 0;

firestore.settings({ timestampsInSnapshots: true });

const auth = app.auth();

exports.storeHeatpumpData = (req, res) => {
    let payload = req.body;
    if (payload && payload.app_id == "freecooling-monitor") {
        const deviceId = payload.dev_id;
        const roomData = payload.payload_fields.room;
        const waterData = payload.payload_fields.water;
        const timestamp = payload.metadata.time;

        try {
            firestore.collection(`${deviceId}`).add({ 'room': roomData, 'water': waterData, 'timestamp': timestamp });
            console.log(`Added data for ${deviceId}`);
        } catch (error) {
            console.error(`error while trying to store data for: ${deviceId}`, error);
        }
        res.status(200).send(deviceId);
    } else {
        res.status(404).send(deviceId);
    }
};

exports.storeBikecounterData = (req, res) => {
    let payload = req.body;
    if (payload && payload.app_id == "bikecounter") {
        const deviceId = payload.dev_id;
        const counter = payload.payload_fields.counter;
        const timestamp = payload.metadata.time;
        try {
            firestore.collection(`${deviceId}`).add({'counter': counter, 'timestamp': timestamp });
            console.log(`Added data for ${deviceId}`);
        } catch (error) {
            console.error(`error while trying to store data for: ${deviceId}`, error);
        }
        res.status(200).send(deviceId);
    } else {
        res.status(404).send(deviceId);
    }
};
