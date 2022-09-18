const admin = require('firebase-admin');
const moment = require('moment');
moment.locale('de');

admin.initializeApp();
const database = admin.firestore();
let healthData = [];

function resetData() {
    healthData = [];
    healthData.push({ "measure": "batteryLevel", "data": [] });
    healthData.push({ "measure": "batteryVoltage", "data": [] });
    healthData.push({ "measure": "temperature", "data": [] });
    healthData.push({ "measure": "humidity", "data": [] });
}

exports.getDeviceHealthData = (request, response) => {

    resetData();
    let collection = request.query.collection;
    if (!collection) {
        collection = "adlisberg-1";
    }
    const countersRef = database.collection(collection);
    let now = new Date();
    let start = moment(now).subtract(1, 'month').format("YYYY-MM-DD");
    let query = countersRef
        .where('timestamp', '>=', start + "T00:00:00")
        .orderBy("timestamp", "asc").get()
        .then(snapshot => {
            if (snapshot.empty) {
                doRespond(404, "<h1>No matching documents.</h1>", response);
            }

            snapshot.forEach(doc => {
                //basic documents have only timestamp and counter -> skip those
                if (Object.keys(doc.data()).length > 2) {
                    let timestamp = new Date(doc.data().timestamp);
                    let batLevel = doc.data().batteryLevel;
                    let batVoltage = doc.data().batteryVoltage;
                    let temperature = doc.data().temperature;
                    let humidity = doc.data().humidity;
                    let bat = { x: timestamp.getTime(), y: batLevel };
                    let volt = { x: timestamp.getTime(), y: batVoltage };
                    let temp = { x: timestamp.getTime(), y: temperature };
                    let hum = { x: timestamp.getTime(), y: humidity };
                    healthData[0].data.push(bat);
                    healthData[1].data.push(volt);
                    healthData[2].data.push(temp);
                    healthData[3].data.push(hum);
                }

            });

            doRespond(200, JSON.stringify(healthData), response, true);

        })
        .catch(err => {
            doRespond(500, "<h1>Error getting documents</h1>", response);
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