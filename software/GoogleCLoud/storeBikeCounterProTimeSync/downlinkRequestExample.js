const https = require('https');
const crypto = require('crypto');

// ----------------------------------
// INPUT PARAMETERS (Please update them before running this script)
// ----------------------------------

const DOWNLINK_HOSTNAME = 'portal.lpn.swisscom.ch'
const DOWNLINK_PATH     = '/thingpark/lrc/rest/v2/downlink'

const AS_KEY              = '9a8e0d4050114bad87019d547477553a'
const AS_ID               = 'TWA_100055533.77834.AS'

const DevEUI              = 'A8610A32314B7505'
const FPort               = 1
const Payload             = '0103'    // Abeeway Position On Demand command
const Confirmed           = false     // optional, Possible values: Ture|False
const FlushDownlinkQueue  = false     // optional, Possible values: Ture|False
const ValidityTime        = undefined // optional, Example: "2018-10-17T16:38:46.882+02:00"
const CorrelationID       = undefined // optional, Example: "1234"

const timeDrift = -2587261;

// ----------------------------------


// ----------------------------------
// Creating the query_string that is part of the request URL and is used to generate the token
// ----------------------------------

const payload_hex = encodeDownlink(timeDrift);

// 'DevEUI', 'FPort' and 'Payload' are mandatory part of the query_string
query_string = 'DevEUI=' + DevEUI + '&FPort=' + FPort.toString() + '&payload=' + payload_hex;

// 'Confirmed', 'FlushDownlinkQueue' and 'ValidityTime' are optional part of the query_string
if (Confirmed) {
    query_string += '&Confirmed=1';
}
if (FlushDownlinkQueue) {
    query_string += '&FlushDownlinkQueue=1';
}
if (ValidityTime) {
    query_string += '&ValidityTime=' + ValidityTime;
}

// 'AS_ID' and 'Time' are mandatory part of the query_string
const nowIso = new Date().toISOString();
query_string += '&AS_ID=' + AS_ID + '&Time=' + nowIso; 

// 'CorrelationID' is optional part of the query_string
if (CorrelationID) {
    query_string += '&CorrelationID=' + CorrelationID;
}

// 'Token' is mandatory part of the query_string
Token = crypto.createHash('sha256').update(query_string + AS_KEY).digest('hex');
query_string += '&Token=' + Token;

// The 'Time' parameter within the query_string includes ':' and '+' characters that have to be encoded
query_string = query_string.replace(/\:/gi, '%3A').replace(/\+/gi, '%2B');

// ----------------------------------

// console.log(query_string);

const req = https.request(
    {
        hostname: DOWNLINK_HOSTNAME,
        path: DOWNLINK_PATH + '?' + query_string,
        method: 'POST',
    },
    res => {
        console.log(`statusCode: ${res.statusCode}`);
        res.on('data', d => {
            console.log(d.toString(), '\n');
        });
    }
)

req.on('error', error => {
    console.error(error)
})
  
req.write('')
req.end()


function encodeDownlink(timeDrift) {
  var seconds = timeDrift >> 0;
  var encodedSeconds = [0, 0, 0, 0];
  encodedSeconds[0] = seconds & 0xff;
  encodedSeconds[1] = (seconds >> 8) & 0xff;
  encodedSeconds[2] = (seconds >> 16) & 0xff;
  encodedSeconds[3] = (seconds >> 24) & 0xff;
  return encodedSeconds.map(b => b.toString(16).padStart(2, '0')).join('');  
}