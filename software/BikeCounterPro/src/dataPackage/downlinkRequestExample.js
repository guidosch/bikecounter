const https = require("https");

const bearerToken =
  "NNSXS.AQOK6SJ6WS4M35KPRLVYTAQ52PE3LC2UJFUR37A.5TJEMDNKGJFJ3VTX27VYRAKJBEBO7G2VOHX5RLDCSJ3ZGGZEG7IQ";
const appId = "bikecounter-dev";
const webhookId = "google-cloud-function";
const deviceId = "test-device-1";

const data = JSON.stringify({
  downlinks: [
    {
      decoded_payload: {
        timeDrift: -2587261,
      },
    },
  ],
});

const options = {
  hostname: "eu1.cloud.thethings.network",
  port: 443,
  path:
    "/api/v3/as/applications/" +
    appId +
    "/webhooks/" +
    webhookId +
    "/devices/" +
    deviceId +
    "/down/replace",
  method: "POST",
  headers: {
    Authorization: "Bearer " + bearerToken,
    "Content-Type": "application/json",
    "Content-Length": data.length,
  },
};

const req = https.request(options, (res) => {
  console.log(`statusCode: ${res.statusCode}`);

  res.on("data", (d) => {
    process.stdout.write(d);
  });
});

req.on("error", (error) => {
  console.error(error);
});

req.write(data);
req.end();
