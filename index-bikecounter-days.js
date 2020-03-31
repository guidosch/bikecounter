const functions = require('firebase-functions');
const admin = require('firebase-admin');
const moment = require('moment');
moment.locale('de');


admin.initializeApp();
const database = admin.firestore();
const countersRef = database.collection('bike-couter-test');
let chartData = {};
let sums = new Map();

function resetData() {
    chartData.cols = [
        {"id":"","label":"Tag","pattern":"","type":"date"},
        {"id":"","label":"Abfahrten","pattern":"","type":"number"}];
    chartData.rows = [];
    sums = new Map();
    for (let i = 1; i <= 31; i++) {
        sums.set(i, 0);
    }
}

exports.printDailyGraphData = functions.https.onRequest((request, response) => {

    resetData();
    //queryTime must be in format 2020-03-22
    let queryTime = request.query.q;
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
        .where('timestamp', '<=', end+"T23:59:59")
        .orderBy("timestamp", "asc").get()
        .then(snapshot => {
            if (snapshot.empty) {
                console.log('No matching documents.');
                response.statusCode = 404;
                response.send("<h1>No matching documents.</h1>");
            }


            snapshot.forEach(doc => {
                let data = doc.data();
                let time = new Date(Date.parse(data.timestamp));
                let sum = sums.get(time.getDate());
                sum = sum + parseInt(data.counter);
                sums.set(time.getDate(), sum);
            });

            // eslint-disable-next-line promise/always-return
            for (let i = 1; i <= lastDayOfMonth; i++) {
                let date = new Date(new Date().setDate(i));
                let year = moment(date).format("YYYY");
                let month = date.getMonth();
                let day  = moment(date).format("DD");
                const sum = sums.get(i);
                let row = `{"c":[{"v":"Date(${year},${month},${day})","f":null},{"v":${sum},"f":null}]}`;
                chartData.rows.push(JSON.parse(row));
            }

            response.statusCode = 200;
            response.setHeader('Access-Control-Allow-Origin', "*");
            response.setHeader('Access-Control-Allow-Methods', 'GET');
            response.setHeader("Content-Type", "application/json");
            response.send(JSON.stringify(chartData));
        })
        .catch(err => {
            console.log('Error getting documents', err);
            response.statusCode = 500;
            response.send("<h1>Error getting documents</h1>");
        });

});