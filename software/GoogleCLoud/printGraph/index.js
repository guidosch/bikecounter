/**
 * Google cloud function for graph data preparation
 */

const functions = require('firebase-functions');
const admin = require('firebase-admin');
const moment = require('moment');
moment.locale('de');


admin.initializeApp();
const database = admin.firestore();
const countersRef = database.collection('bike-couter-test');
let chartData = {};
let monthlySum = new Map();

function resetData() {
    chartData.cols = [
        { "id": "", "label": "Monat", "pattern": "", "type": "string" },
        { "id": "", "label": "Abfahrten", "pattern": "", "type": "number" }];
    chartData.rows = [];
    monthlySum = new Map();
    for (let i = 0; i < 12; i++) {
        monthlySum.set(i, 0);
    }
}

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

exports.printGraph = functions.https.onRequest((request, response) => {

    resetData();
    //queryTime must be in format 2020-03-22 for march, 22
    let queryTime = request.query.q;

    let collection = request.query.collection;
    if (!collection) {
        collection = "bike-couter-test";
    }

    const countersRef = database.collection(collection);

    let now = new Date();
    let start = moment(now).startOf('year').format("YYYY-MM-DD");
    let end = moment(now).endOf('year').format("YYYY-MM-DD");
    if (queryTime) {
        start = moment(new Date(queryTime)).startOf('year').format("YYYY-MM-DD");
        end = moment(new Date(queryTime)).endOf('year').format("YYYY-MM-DD");
    }

    let query = countersRef
        .where('timestamp', '>=', start)
        .where('timestamp', '<=', end + "T23:59:59")
        .orderBy("timestamp", "asc").get()
        .then(snapshot => {
            if (snapshot.empty) {
                doRespond(404, "<h1>No matching documents.</h1>", response);
            }

            snapshot.forEach(doc => {
                let data = doc.data();
                let time = new Date(Date.parse(data.timestamp));
                let sum = monthlySum.get(time.getMonth());
                sum = sum + parseInt(data.counter);
                monthlySum.set(time.getMonth(), sum);
            });

            // eslint-disable-next-line promise/always-return
            for (let i = 0; i < 12; i++) {
                let date = new Date(new Date().setMonth(i));
                //const month = date.toLocaleString('de', { month: 'long' });
                const month = moment(date).format("MMMM");
                const sum = monthlySum.get(i);
                let row = (`{"c":[{"v":"${month}","f":null},{"v":${sum},"f":null}]}`);
                chartData.rows.push(JSON.parse(row));
            }

            doRespond(200, JSON.stringify(chartData), response, true);
        })
        .catch(err => {
            doRespond(500, "<h1>Error getting documents</h1>", response);
        });

});