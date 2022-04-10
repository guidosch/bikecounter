var fs = require('fs');
var data = JSON.parse(fs.readFileSync('./ttnV3DeviceUploadData.json', 'utf8'));
//console.log(obj);

const app_id = data.end_device_ids.application_ids.application_id;
const deviceId = data.end_device_ids.device_id;
const forward_temp = data.uplink_message.decoded_payload.forward_temp;
const return_temp = data.uplink_message.decoded_payload.return_temp;
const timestamp = data.received_at;
const gatways = data.uplink_message.rx_metadata[0].gateway_ids;



console.log("app_id: "+app_id);
console.log("deviceId: "+deviceId);
console.log("forward_temp: "+forward_temp);
console.log("return_temp: "+return_temp);
console.log("timestamp: "+timestamp);


console.log("gatways: "+gatways.gateway_id);
