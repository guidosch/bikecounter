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


exports.printGraphData = functions.https.onRequest((request, response) => {

    let collection = request.query.collection;
    const countersRef = database.collection(collection);
    chartData.rows = [];
    let oneMonthAgo = new Date();
    oneMonthAgo.setDate(oneMonthAgo.getDate() - 30);
    let query = countersRef.where('timestamp', '>', oneMonthAgo.toISOString()).orderBy("timestamp", "asc").get()

        .then(snapshot => {
            if (snapshot.empty) {
                console.log('No matching documents.');
                response.statusCode = 404;
                response.send("<h1>No matching documents.</h1>");
            }

            snapshot.forEach(doc => {
                let data = doc.data();
                let time = new Date(Date.parse(data.timestamp));
                correctSensorData(collection, data);
                let forwardTemp = data.forward;
                let returnTemp = data.return;
                let diff = Math.round(returnTemp - forwardTemp);
                let row = (`
                    {"c":[{"v":"Date(${time.getFullYear()},${time.getMonth()},${time.getDate()},${time.getHours()},${time.getMinutes()})"},
                    {"v":${forwardTemp}},
                    {"v":${returnTemp}},
                    {"v":${diff}}]}
                `);
                chartData.rows.push(JSON.parse(row));
            });

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

//some sensors are off - correct values
function correctSensorData(collection, data) {
    switch (collection) {
        case "t75-monitor":
            data.forward = data.forward + 1;
            break;
        case "t77-monitor":
            data.return = data.return + 2;
            break;
    }

}