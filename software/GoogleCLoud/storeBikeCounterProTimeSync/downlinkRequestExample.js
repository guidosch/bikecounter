const https = require("https");

const bearerToken = "xxx";
const deviceId = "A8610A32314B7505";
const timeDrift = -2587261;

function encodeDownlink(timeDrift) {
  var seconds = timeDrift >> 0;
  var encodedSeconds = [0, 0, 0, 0];
  encodedSeconds[0] = seconds & 0xff;
  encodedSeconds[1] = (seconds >> 8) & 0xff;
  encodedSeconds[2] = (seconds >> 16) & 0xff;
  encodedSeconds[3] = (seconds >> 24) & 0xff;
  encodedSecondsHexString = encodedSeconds.map(b => b.toString(16).padStart(2, '0')).join('');
  
  console.log("Encoded seconds hex string: " + encodedSecondsHexString);

  return JSON.stringify({
    "DevEUI_downlink": {
      "Time": new Date().toISOString(),
      "DevEUI": deviceId,
      "FPort": 1,
      "payload_hex": encodedSecondsHexString
    }
  });
}

const data = encodeDownlink(timeDrift);

const options = {
  hostname: "portal.lpn.swisscom.ch",
  port: 443,
  path: "/thingpark/lrc/rest/v2/downlink/",
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
