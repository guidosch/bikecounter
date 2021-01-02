/**
 * Google cloud function for graph data preparation
 */

const functions = require('firebase-functions');
const admin = require('firebase-admin');

admin.initializeApp();
const database = admin.firestore();
let chartData = {};
chartData.cols = [
    { "id": "", "label": "Day", "pattern": "", "type": "date" },
    { "id": "", "label": "Vorlauf", "pattern": "", "type": "number" },
    { "id": "", "label": "Ruecklauf", "pattern": "", "type": "number" },
    { "id": "", "label": "Differenz", "pattern": "", "type": "number" }
];

function doRespond(statusCode, responseString, response, asJson) {
    response.statusCode = statusCode;
    response.setHeader('Access-Control-Allow-Origin', "*");
    response.setHeader('Access-Control-Allow-Methods', 'GET');
    if (asJson){
        response.setHeader("Content-Type", "application/json");
    } else {
        response.setHeader("Content-Type", "text/html");
    }
    response.send(responseString);
}

exports.printGraphData = functions.https.onRequest((request, response) => {

    let collection = request.query.collection;
    const countersRef = database.collection(collection);
    chartData.rows = [];
    let oneMonthAgo = new Date();
    oneMonthAgo.setDate(oneMonthAgo.getDate() - 30);
    let query = countersRef.where('timestamp', '>', oneMonthAgo.toISOString()).orderBy("timestamp", "asc").get()

        .then(snapshot => {
            if (snapshot.empty) {
                doRespond(404, "<h1>No matching documents.</h1>", response);
            }

            snapshot.forEach(doc => {
                let data = doc.data();
                let time = new Date(Date.parse(data.timestamp));
                correctSensorData(collection, data);
                let forwardTemp = data.forward;
                let returnTemp = data.return;
                let diff = returnTemp - forwardTemp;
                let row = (`
                    {"c":[{"v":"Date(${time.getFullYear()},${time.getMonth()},${time.getDate()},${time.getHours()},${time.getMinutes()})"},
                    {"v":${forwardTemp}},
                    {"v":${returnTemp}},
                    {"v":${diff}}]}
                `);
                chartData.rows.push(JSON.parse(row));
            });

            doRespond(200, JSON.stringify(chartData), response, true);
        })
        .catch(err => {
            doRespond(500, "<h1>Error getting documents</h1>", response);
        });
});

//some sensors are off - correct values
function correctSensorData(collection, data) {
    switch (collection) {
        case "t75-monitor":
            data.forward = data.forward + 0.75;
            break;
        case "t77-monitor":
            data.return = data.return + 1.25;
            break;
    }

}