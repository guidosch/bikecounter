const functions = require('firebase-functions');
const admin = require('firebase-admin');

admin.initializeApp();
const database = admin.firestore();
const countersRef = database.collection('t73-monitor');
let chartData = {};
chartData.cols = [
    { "id": "", "label": "Day", "pattern": "", "type": "date" },
    { "id": "", "label": "Water", "pattern": "", "type": "number" },
    { "id": "", "label": "Air", "pattern": "", "type": "number" },
    { "id": "", "label": "Diff", "pattern": "", "type": "number" }
];


exports.printGraphData = functions.https.onRequest((request, response) => {

    chartData.rows = [];
    let oneWeekAgo = new Date();
    oneWeekAgo.setDate(oneWeekAgo.getDate()-7);
    let query = countersRef.where('timestamp', '>', oneWeekAgo.toISOString()).orderBy("timestamp", "asc").get()

        .then(snapshot => {
            if (snapshot.empty) {
                console.log('No matching documents.');
                response.statusCode = 404;
                response.send("<h1>No matching documents.</h1>");
            }

            snapshot.forEach(doc => {
                let data = doc.data();
                let time = new Date(Date.parse(data.timestamp));
                time.get
                let tempWater = data.water.t;
                let tempRoom = data.room.t;
                let diff = Math.round(tempRoom - tempWater);
                let row = (`
                    {"c":[{"v":"Date(${time.getFullYear()},${time.getMonth()},${time.getDate()},${time.getHours()},${time.getMinutes()})"},
                    {"v":${tempWater}},
                    {"v":${tempRoom}},
                    {"v":${diff}}]}
                `);
                chartData.rows.push(JSON.parse(row));
            });

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