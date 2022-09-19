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
    if (payload && app_id == "bikecounterzueritrails") {
        deviceId = payload.end_device_ids.device_id;
        const counter = payload.uplink_message.decoded_payload.counter;
        const timestamp = payload.received_at;

        //if (!alreadyAdded(deviceId, timestamp, 10)) { may be no longer needed

            try {
                firestore.collection(`${deviceId}`).add({'counter': counter, 'timestamp': timestamp });
                console.log(`Added data for ${deviceId}`);
            } catch (error) {
                console.error(`error while trying to store data for: ${deviceId}`, error);
            }
        //}
        res.status(200).send(deviceId);
    } else {
        console.error("payload not valid: "+JSON.stringify(payload));
        res.status(404).send(deviceId);
    }
};

function alreadyAdded(deviceId, timestamp, minutes) {
    let time = new Date(Date.parse(timestamp));
    let minutesAgo = new Date(time.getTime() - (minutes * 60 * 1000));
    let alreadyAdded = false;

    firestore.collection(`${deviceId}`)
        .where("timestamp", ">", minutesAgo.toISOString())
        .orderBy("timestamp", "desc")
        .get()
        .then(function (querySnapshot) {
            querySnapshot.forEach(function (doc) {
                let data = doc.data();
                let time = new Date(Date.parse(data.timestamp));
                alreadyAdded = true;
                console.log("Data already added - skipping entry with timestamp: " + time.toISOString());
            });
        })
        .catch(function (error) {
            console.log("Error getting already added documents: ", error);
        });
    return alreadyAdded;
}
