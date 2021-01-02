const functions = require('firebase-functions');
const admin = require('firebase-admin');
const moment = require('moment');
moment.locale('de');


admin.initializeApp();
const database = admin.firestore();
let chartData = {};
let sums = new Map();
let currentTimeFromData;

function resetData() {
    chartData.cols = [
        { "id": "", "label": "Tag", "pattern": "", "type": "date" },
        { "id": "", "label": "Abfahrten", "pattern": "", "type": "number" }];
    chartData.rows = [];
    sums = new Map();
    for (let i = 1; i <= 31; i++) {
        sums.set(i, 0);
    }
}

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

exports.printDailyGraphData = functions.https.onRequest((request, response) => {

    resetData();
    //queryTime must be in format 2020-03-22 for march, 22
    let queryTime = request.query.q;
    let collection = request.query.collection;
    if (!collection) {
        collection = "bike-couter-test";
    }
    const countersRef = database.collection(collection);
    let now = new Date();
    let start = moment(now).startOf('month').format("YYYY-MM-DD");
    let end = moment(now).endOf('month').format("YYYY-MM-DD");
    let lastDayOfMonth = parseInt(moment(now).endOf('month').format("DD"));
    if (queryTime) {
        start = moment(new Date(queryTime)).startOf('month').format("YYYY-MM-DD");
        end = moment(new Date(queryTime)).endOf('month').format("YYYY-MM-DD");
        lastDayOfMonth = parseInt(moment(new Date(queryTime)).endOf('month').format("DD"));
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
                currentTimeFromData = new Date(Date.parse(data.timestamp));
                let sum = sums.get(currentTimeFromData.getDate());
                sum = sum + parseInt(data.counter);
                sums.set(currentTimeFromData.getDate(), sum);
            });

            // eslint-disable-next-line promise/always-return
            for (let i = 1; i <= lastDayOfMonth; i++) {
                let date = new Date(currentTimeFromData.setDate(i));
                let year = moment(date).format("YYYY");
                let month = date.getMonth();
                let day = moment(date).format("DD");
                const sum = sums.get(i);
                let row = `{"c":[{"v":"Date(${year},${month},${day})","f":null},{"v":${sum},"f":null}]}`;
                chartData.rows.push(JSON.parse(row));
            }

            doRespond(200, JSON.stringify(chartData), response, true);

        })
        .catch(err => {
            doRespond(500, "<h1>Error getting documents</h1>", response);
        });

});