const admin = require('firebase-admin');
const moment = require('moment');

admin.initializeApp();
const oneDayBack = new Date(new Date().getTime() - (1000 * 60 * 60 * 24));
const db = admin.firestore();

exports.getDevicesSummaryPro = (req, res) => {
    let devicesSummary = [];
    db.listCollections().then(collections => {
        if (collections.empty) {
            doRespond(404, "<h1>No matching documents.</h1>", res);
        }
        let i = 0;
        collections.forEach(collection => {
            console.log("Query device with ID: "+collection.id);
            db.collection(collection.id)
                .where("timestamp", ">=", oneDayBack.toISOString())
                .orderBy("timestamp", "asc").get()
                .then(snapshot => {
                    let device = {};
                    //collection is an object not like when querying a single known collection
                    device.id = collection.id;
                    device.sumLast24h = 0;
                    device.sumToday = 0;
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
                    console.log("Added device: "+JSON.stringify(device));
                    devicesSummary.push(device);
                    if (collections.length === ++i){
                        devicesSummary.sort(function(a,b){
                            if (a.id < b.id){
                                return -1;
                            }
                            if (a.id > b.id){
                                return 1;
                            }
                        });
                        doRespond(200, JSON.stringify(devicesSummary), res, true);
                    }
                    
                }).catch(err => {
                    console.log('Error getting documents', err);
                    doRespond(500, "<h1>Error getting documents</h1>", res);
                });

            });
            
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