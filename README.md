# LORA data to Google Chart
Using Google cloud functions to publish [LORA](https://en.wikipedia.org/wiki/LoRa) data to a firestore database and also offer the data over a second cloud function to be visualized in a chart. Data is recoreded with a LORA module and sent to the things network and there forwared to a the Google Cloud Function.

## Dataflow
1. Recodring data with a [Arduino MKRWAN 1310](https://www.arduino.cc/en/Guide/MKRWAN1310) LORA modul and sending over [TTN network](https://www.thethingsnetwork.org/)
2. Using TTN [HTTP integration](https://www.thethingsnetwork.org/docs/applications/http/) to forward TTN data to my Google [Cloud Function](https://console.cloud.google.com/functions/list?project=bikecounter)
3. Google Cloud Function pushes data from TTN to a firestore database
    * https://us-central1-bikecounter.cloudfunctions.net/storeBikeCounterData
4. Using a second Google Cloud Function to offer an API which can be used with [Google Chart API](https://developers.google.com/chart/interactive/docs).
    * https://us-central1-bikecounter.cloudfunctions.net/printGraph
5. Display data in a HTML page with chart served over github pages
    * https://guidosch.github.io/lora/bikecounter.html


The repo contains all the code from the steps above.