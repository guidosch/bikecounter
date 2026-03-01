# Flow from Device to TTN

1. Device sends data to TTN via LoRaWAN as uplink message encoded in bytes.
2. TTN receives the uplink message and processes it via the payload formatter (uplink)
3. The result is forwarded to GCP cloud function: See uplink_example.js for an example of the processed data.




# Flow from TTN to Device