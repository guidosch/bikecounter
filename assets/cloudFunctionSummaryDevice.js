const admin = require('firebase-admin');

admin.initializeApp();
const oneDayBack = new Date(new Date().getTime() - (1000 * 60 * 60 * 24));
const db = admin.firestore();



exports.getDevicesSummaryPro = (req, res) => {
    let deviceSummary = [];

    let collection = request.query.collection;
    if (!collection) {
        doRespond(500, "<h1>No collection query attribute found!</h1>", res);
        return;
    }
    const collectionRef = database.collection(collection);

    collectionRef.where("timestamp", ">", oneDayBack.toISOString())
        .orderBy("timestamp", "asc").get()
        .then(snapshot => {
            let device = {};
            device.id = collection.id;
            device.sumLast24h = null;
            device.battery = null;
            device.temperature = null;
            device.humidity = null;
            device.gateways = null;
            device.location = null;
            device.status = null;

            if (snapshot.empty) {
                device.online = false;
            }

            snapshot.forEach(doc => {
                device.sumLast24h += doc.data().counter;
                device.timestampLastMsg = doc.data().timestamp;
                device.online = true;
                if (Object.keys(doc.data()).length > 2) {
                    device.battery = doc.data().batteryLevel;
                    device.temperature = doc.data().temperature;
                    device.humidity = doc.data().humidity;
                    device.gateways = doc.data().gateways;
                    device.location = doc.data().location;
                    device.status = doc.data().stat;
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