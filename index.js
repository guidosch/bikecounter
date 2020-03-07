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
        {"id":"","label":"Monat","pattern":"","type":"string"},
        {"id":"","label":"Abfahrten","pattern":"","type":"number"}];
    chartData.rows = [];
    monthlySum = new Map();
    for (let i = 0; i < 12; i++) {
        monthlySum.set(i, 0);
    }
}

exports.printGraph = functions.https.onRequest((request, response) => {

    resetData();
    let query = countersRef.where('timestamp', '>', '2020-01-01').get()
        .then(snapshot => {
            if (snapshot.empty) {
                console.log('No matching documents.');
                response.statusCode = 404;
                response.send("<h1>No matching documents.</h1>");
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

            response.statusCode = 200;
            response.setHeader('Access-Control-Allow-Origin', "*")
            response.setHeader('Access-Control-Allow-Methods', 'GET')
            response.setHeader("Content-Type", "application/json");
            response.send(JSON.stringify(chartData));
        })
        .catch(err => {
            console.log('Error getting documents', err);
            response.statusCode = 500;
            response.send("<h1>Error getting documents</h1>");
        });

});