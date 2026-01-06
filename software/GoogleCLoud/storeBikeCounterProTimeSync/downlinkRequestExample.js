const https = require("https");
const crypto = require("crypto");

const deviceId = "A8610A32314B7505";
const timeDrift = -2587261;
const AS_ID = "TWA_100055533.77834.AS";
const nowIso = new Date().toISOString();

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
      "AS_ID": AS_ID,
      "Time": nowIso,
      "DevEUI": deviceId,
      "FPort": 1,
      "payload_hex": encodedSecondsHexString
    }
  });
}

const data = encodeDownlink(timeDrift);

console.log("Downlink payload: " + data);

const TIAK = "xxx"; //read from ENV

const fPort = "1";


const queryParams = {
    AS_ID: AS_ID,
    DevEUI: deviceId,
    FPort: fPort,
    Time: nowIso,
    payload_hex: encodedSecondsHexString
};
const sortedKeys = Object.keys(queryParams).sort();
const queryStringForSig = sortedKeys.map(k => `${k}=${queryParams[k]}`).join('&');

// Build the signature input (body + '&' + query + '&' + key) —
// exact separators may differ in your tenant’s doc; this is a clear, consistent approach.
const sigInput = `${data}&${queryStringForSig}&${TIAK}`;
const token = crypto.createHash('sha256').update(sigInput, 'utf8').digest('hex');

const options = {
    hostname: "portal.lpn.swisscom.ch",
    port: 443,
    path: `/thingpark/lrc/rest/v2/downlink?${queryStringForSig}&Token=${token}`,
    method: "POST",
    headers: {
        //"Content-Type": "application/json",
        "Content-Type": "application/x-www-form-urlencoded",
        //"Content-Length": data.length,
    },
};

const req = https.request(options, (res) => {
  console.log(`statusCode: ${res.statusCode}`);
  console.log("Headers: ", res.headers);
  
  let body = "";
  res.on("data", (d) => {
    body += d;
  });
  
  res.on("end", () => {
    console.log("Response body: ", body);
  });
  
});


req.on("error", (error) => {
  console.error(error);
});

req.write();
req.end();
