const admin = require('firebase-admin');
const moment = require('moment');

admin.initializeApp();
const oneDayBack = new Date(new Date().getTime() - (1000 * 60 * 60 * 24));
const db = admin.firestore();

exports.getDeviceSummaryPro = (req, res) => {
    let deviceSummary = [];
    let collection = req.query.collection;
    if (!collection) {
        doRespond(500, "<h1>No collection query attribute found!</h1>", res);
        return;
    }
    const collectionRef = db.collection(collection);
    collectionRef.where("timestamp", ">=", oneDayBack.toISOString())
        .orderBy("timestamp", "asc").get()
        .then(snapshot => {
            let device = {};
            device.id = collection;
            device.sumLast24h = null;
            device.sumToday = null;
            device.batteryLevel = null;
            device.batteryVoltage = null;
            device.temperature = null;
            device.humidity = null;
            device.gateways = null;
            device.location = null;
            device.status = null;
            device.swVersion = null;
            device.hwVersion = null;

            if (snapshot.empty) {
                device.online = false;
            }

            snapshot.forEach(doc => {
                device.sumLast24h += doc.data().counter;
                device.sumToday += isCountDataFromToday(doc.data().counter, doc.data().timestamp);
                device.timestampLastMsg = doc.data().timestamp;
                device.online = true;
                if (Object.keys(doc.data()).length > 2) {
                    device.batteryLevel = doc.data().batteryLevel;
                    device.batteryVoltage = doc.data().batteryVoltage;
                    device.temperature = doc.data().temperature;
                    device.humidity = doc.data().humidity;
                    device.gateways = doc.data().gateways;
                    device.location = doc.data().location;
                    device.status = doc.data().stat;
                    device.swVersion = doc.data().swVersion;
                    device.hwVersion = doc.data().hwVersion;
                }
            });
            console.log("Added device: " + JSON.stringify(device));
            deviceSummary.push(device);
            doRespond(200, JSON.stringify(deviceSummary), res, true);

        }).catch(err => {
            console.log('Error getting documents', err);
            doRespond(500, "<h1>Error getting documents</h1>", res);
        });

};

function doRespond(statusCode, responseString, response, asJson) {
    response.statusCode = statusCode;
    response.setHeader('Access-Control-Allow-Origin', "*");
    response.setHeader('Access-Control-Allow-Methods', 'GET');
    if (asJson) {
        response.setHeader("Content-Type", "application/json");
    } else {
        response.setHeader("Content-Type", "text/html");
    }
    response.send(responseString);
}

function isCountDataFromToday(count, timestamp) {
    let sameDay = moment().isSame(timestamp, 'day'); 
    if (sameDay) {
        return count;
    }
    return 0;
     
}