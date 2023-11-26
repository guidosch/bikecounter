const functions = require("firebase-functions"),
  admin = require("firebase-admin");
const https = require("https");

const app = admin.initializeApp();
const firestore = app.firestore();
const db = admin.firestore();
var num = 0;

firestore.settings({ timestampsInSnapshots: true });

const auth = app.auth();
const ttnWebhookId = "google-cloud-function";

exports.storeBikecounterData = (req, res) => {
  let payload = req.body;
  let deviceId;
  let deviceEUI;
  const app_id = payload.end_device_ids.application_ids.application_id;
  if (payload) {
    deviceId = payload.end_device_ids.device_id;
    deviceEUI = payload.end_device_ids.dev_eui;
    const devicePayload = payload.uplink_message.decoded_payload;
    const batteryLevel = devicePayload.batteryLevel;
    const batteryVoltage = devicePayload.batteryVoltage;
    const humidity = devicePayload.humidity;
    const temperature = devicePayload.temperature;
    const statId = devicePayload.statId;
    const stat = devicePayload.stat;
    const swVersion = devicePayload.swVersion;
    const hwVersion = devicePayload.hwVersion;
    const timeArray = devicePayload.timeArray;
    const timeDrift = devicePayload.timeDrift;
    let transmissionTime = devicePayload.deviceTransmissionTime;
    if (transmissionTime) {
      //is sent as seconds since 1970 UTC
      transmissionTime = transmissionTime * 1000;
    } else {
      transmissionTime = new Date().getTime();
    }

    //create a map with unique timestamps as keys and sum up the counts (one timestamp per trigger) per timestamp
    let map = new Map();
    timeArray.forEach((t) => {
      if (!map.get(t)) {
        map.set(t, 1);
      } else {
        map.set(t, map.get(t) + 1);
      }
    });

    // LoRa network informations
    let gateways = [];
    for (let gateway of payload.uplink_message.rx_metadata) {
      gateways.push({
        id: gateway.gateway_ids.gateway_id,
        eui: gateway.gateway_ids.eui,
        rssi: gateway.rssi,
        snr: gateway.snr,
      });
    }
    const airtime = payload.uplink_message.consumed_airtime;

    // statId == 7 is the time sync call
    if (app_id == "bikecounter" && statId != 7) {
      try {
        // get the collection id (trail) from the deviceEUI
        db.collection("internal-deviceId-trail-ct")
          .where("deviceEUI", "==", deviceEUI)
          .orderBy("validFrom")
          .limit(1)
          .get()
          .then((snapshot) => {
            if (snapshot.empty) {
              console.log("No trail for device found!");
            } else {
              snapshot.forEach((doc) => {
                console.log(doc.data().collectionID);
                const collId = doc.data().collectionID;

                // store the parsed payload into the trail collection
                firestore.collection(`${collId}`).add({
                  counter: 0,
                  timestamp: new Date(transmissionTime).toISOString(),
                  batteryLevel: batteryLevel,
                  batteryVoltage: batteryVoltage,
                  humidity: humidity,
                  temperature: temperature,
                  stat: stat,
                  gateways: gateways,
                  airtime: airtime,
                  swVersion: swVersion,
                  hwVersion: hwVersion,
                });
                console.log(`Added health data for ${collId}`);

                //one more DB entry for every timestamp
                for (let timestamp of map.keys()) {
                  let date = new Date(timestamp).toISOString();
                  firestore
                    .collection(`${collId}`)
                    .add({ counter: map.get(timestamp), timestamp: date });
                  console.log(`Added data for ${collId}`);
                }
              });
            }
          })
          .catch((err) => {
            console.error("Error while performing queries! ", err);
          });
      } catch (error) {
        console.error(
          `error while trying to store data for: ${deviceId}`,
          error
        );
      }
    }

    // send downlink package with timeDrift information
    if (Math.abs(timeDrift) > 15 * 60) {
      // create package data
      const data = JSON.stringify({
        downlinks: [
          {
            decoded_payload: {
              timeDrift: timeDrift,
            },
          },
        ],
      });

      // POST request options
      const replaceURL = req.headers["x-downlink-replace"];

      const options = {
        hostname: "eu1.cloud.thethings.network",
        port: 443,
        path: replaceURL,
        method: "POST",
        headers: {
          Authorization: "Bearer " + req.headers["x-downlink-apikey"],
          "Content-Type": "application/json",
          "Content-Length": data.length,
        },
      };

      //console.log("options: "+JSON.stringify(options));

      // perform the post request to the ttn webhook
      const reqDown = https.request(options, (resDown) => {
        console.log(`statusCode: ${res.statusCode}`);

        resDown.on("data", (d) => {
          process.stdout.write(d);
        });
      });

      reqDown.on("error", (error) => {
        console.error(error);
      });

      reqDown.write(data);
      reqDown.end();
    }

    res.status(200).send(deviceId);
  } else {
    //console.error("payload not valid: " + JSON.stringify(payload));
    res.status(404).send(deviceId);
  }
};
