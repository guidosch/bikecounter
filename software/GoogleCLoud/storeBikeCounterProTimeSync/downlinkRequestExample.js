const https = require('https');
const crypto = require('crypto');

// ----------------------------------
// INPUT PARAMETERS (Please update them before running this script)
// ----------------------------------

const DOWNLINK_HOSTNAME = 'portal.lpn.swisscom.ch';
const DOWNLINK_PATH = '/thingpark/lrc/rest/v2/downlink';

const THINKPARK_AS_KEY = '9a8e0d4050114bad87019d547477553a';
const THINKPARK_AS_ID = 'TWA_100055533.77834.AS';

const DevEUI = 'A8610A32314B7505';
const FPort = 1;
const Confirmed = false;     // optional, Possible values: True|False
const FlushDownlinkQueue = false;     // optional, Possible values: True|False
const ValidityTime = undefined; // optional, Example: "2018-10-17T16:38:46.882+02:00"
const CorrelationID = undefined; // optional, Example: "1234"

const timeDrift = -2587261;

// ----------------------------------
// HELPER FUNCTIONS
// ----------------------------------

/**
 * Encodes time drift value into a hex payload
 */
function encodeDownlink(timeDrift) {
  const seconds = timeDrift >> 0;
  const encodedBytes = encodeInt32ToBytes(seconds);
  return bytesToHex(encodedBytes);
}

/**
 * Encodes a 32-bit integer into 4 bytes (little-endian)
 */
function encodeInt32ToBytes(value) {
  return [
    value & 0xff,
    (value >> 8) & 0xff,
    (value >> 16) & 0xff,
    (value >> 24) & 0xff
  ];
}

/**
 * Converts byte array to hex string
 */
function bytesToHex(bytes) {
  return bytes.map(b => b.toString(16).padStart(2, '0')).join('');
}

/**
 * Builds the base query string with mandatory parameters
 */
function buildBaseQueryString(devEUI, fPort, payload, asId, timestamp) {
  return `DevEUI=${devEUI}&FPort=${fPort}&payload=${payload}&AS_ID=${asId}&Time=${timestamp}`;
}

/**
 * Adds optional parameters to query string
 */
function addOptionalParams(queryString, confirmed, correlationID) {
  let result = queryString;
  
  if (confirmed) {
    result += '&Confirmed=1';
  }
  
  if (correlationID) {
    result += `&CorrelationID=${correlationID}`;
  }
  
  return result;
}

/**
 * Generates SHA256 token for authentication
 */
function generateToken(queryString, asKey) {
  return crypto.createHash('sha256').update(queryString + asKey).digest('hex');
}

/**
 * URL encodes special characters in the query string
 */
function urlEncodeQueryString(queryString) {
  return queryString.replace(/:/gi, '%3A').replace(/\+/gi, '%2B');
}

/**
 * Builds complete authenticated query string
 */
function buildAuthenticatedQueryString(params) {
  const { devEUI, fPort, payload, asId, timestamp, asKey, confirmed, correlationID } = params;
  
  let queryString = buildBaseQueryString(devEUI, fPort, payload, asId, timestamp);
  queryString = addOptionalParams(queryString, confirmed, correlationID);
  
  const token = generateToken(queryString, asKey);
  queryString += `&Token=${token}`;
  
  //must be after token generation
  return urlEncodeQueryString(queryString);
}

/**
 * Handles the HTTP response
 */
function handleResponse(res) {
  console.log(`statusCode: ${res.statusCode}`);
  res.on('data', d => {
    console.log(d.toString(), '\n');
  });
}

/**
 * Handles request errors
 */
function handleError(error) {
  console.error(error);
}

/**
 * Sends downlink request to the server
 */
function sendDownlinkRequest(hostname, path, queryString) {
  const req = https.request(
    {
      hostname: hostname,
      path: path + '?' + queryString,
      method: 'POST',
    },
    handleResponse
  );

  req.on('error', handleError);
  req.write('');
  req.end();
}

// ----------------------------------
// MAIN EXECUTION
// ----------------------------------

function main() {
  const payload_hex = encodeDownlink(timeDrift);
  const nowIso = new Date().toISOString();

  const queryString = buildAuthenticatedQueryString({
    devEUI: DevEUI,
    fPort: FPort,
    payload: payload_hex,
    asId: THINKPARK_AS_ID,
    timestamp: nowIso,
    asKey: THINKPARK_AS_KEY,
    confirmed: Confirmed,
    correlationID: CorrelationID
  });

  sendDownlinkRequest(DOWNLINK_HOSTNAME, DOWNLINK_PATH, queryString);
}

main();