# BikeCounter  <!-- omit in toc -->

This repository contains the hardware and software components of a [PIR](https://en.wikipedia.org/wiki/Passive_infrared_sensor) based tracking device to monitor the usage of local bike trails. The data is sent over [LoRaWAN](https://de.wikipedia.org/wiki/Long_Range_Wide_Area_Network) to [TTN](https://www.thethingsnetwork.org/) and from there to a [Google cloud](https://console.cloud.google.com) backend which stores the data and provides api endpoints for the data visualization web UI ([bikeCounterUI](https://github.com/guidosch/bikecounterUI)).

<img src="hardware/img/BikeCounter_v0.1_1.png" width="400"> <img src="hardware/img/BikeCounter_v0.1_2.png" width="400">
<img src="hardware/img/BikeCounter_v0.1_3.png" width="400"> <img src="hardware/img/BikeCounter_v0.1_4.png" width="400">

The repository is divided into two main sections. The first one contains all the data regarding the hardware components and the second one holds the software code.

- [Hardware](#hardware)
- [Software](#software)
  - [BikeCounter](#bikecounter)
    - [Overall state machine](#overall-state-machine)
    - [dataPackage](#datapackage)
    - [timeScheduler](#timescheduler)
  - [TTN](#ttn)
  - [Google Cloud](#google-cloud)

# Hardware
The core component of the device is an [Arduino MRKWAN 1310](https://docs.arduino.cc/hardware/mkr-wan-1310) which sits on the "logic layer" PCB. This PCB connects the micro controller to the sensors, the dip switches and the "power layer" PCB.

To detect the movement a PIR sensor from [Seeed Studio](https://www.seeedstudio.com/Grove-PIR-Motion-Sensor.html) is used and to monitor the internal environment of the device a temperature and humidity sensor from [Adafruit](https://www.adafruit.com/product/3721) was chosen. 

The "power layer" PCB holds up to two rechargeable lithium-ion batteries (18650). This allows the device to run up to one year without exchanging the batteries.

The schematics and layout files of the PCBs as well as the BOM of the whole device can be found in the corresponding subfolder.

# Software
This Repository contains the code for the tracking device as well as the scripts running on the backend.

**Overview**

```mermaid
flowchart LR;
  subgraph BikeCounter;
    Biker(Round) -->|infrared radiation| PIR-sensor;
    PIR-sensor -->|DIO| Arduino-MKRWAN-1310;
    Temp&Hum-sensor -->|I2C| Arduino-MKRWAN-1310;
    DIP-switches -->|DIO| Arduino-MKRWAN-1310;
  end; 
  subgraph TTN;   
    Arduino-MKRWAN-1310 -->|LoRaWAN| PayloadFormatter;
  end;  
  subgraph Google Cloud;
    PayloadFormatter -->|webhook| Cloud-function;
    Cloud-function --> firebase;
  end;
```
## BikeCounter
There are two versions of the main BikeCounter software. The **light** version is a simple and basic implementation of the needed functionality. The **pro** version has extended features that allow it to take track of time as well as its environnement conditions.

(Due to the fact that the light version is allmost obsolet only the pro version will be explained in more detail.)

### Overall state machine

```mermaid
stateDiagram-v2
    m1: main loop
    [*] --> Setup
    Setup --> m1
    state m1 {
      state if_motion <<choice>>
      s0: motion detected ?
      s1: store information in variable      
      s2: timer called
      s3: threshold reached ?
      state if_threshold <<choice>>
      s4: send data
      state if_downlink <<choice>>
      s5: downlink received ?
      s6: correct rtc drift
      
      [*] --> s0
      s0 --> if_motion
      if_motion --> s1: true
      if_motion --> s2: false
      s2 --> s4
      s1 --> s3
      s3 --> if_threshold
      if_threshold --> deepSleep: false
      if_threshold --> s4: true
      s4 --> s5
      s5 --> if_downlink
      if_downlink --> deepSleep: false
      if_downlink --> s6: true
      s6 --> deepSleep
      deepSleep --> [*]
    }
```
After the main loop finishes the device goes into a deepSleep mode. From there the PIR sensor or the rtc timer can wake up the device and trigger the main loop again.

### dataPackage

### timeScheduler


## TTN
The message from the device gets over LoRaWAN to the TTN server where a uplink payload formatter parses the data bytes to a human readable JavaScript object. The server then triggers a webhook and passes the data object to the Google Cloud function API endpoint.

The downlink message with the time drift information will also be parsed by the payload formatter defined in TTN.

The up/down-link payload formatter scripts are stored in the TTN subfolder.

## Google Cloud
The triggered cloud function evaluates the data object sent from TTN and saves it to the Firebase database. Depending on the difference between the local time of the device and the server time it schedules a downlink message to correct the internal device time.

There are also cloud functions that provide an API endpoint for the web UI ([bikeCounterUI](https://github.com/guidosch/bikecounterUI)) to visualize the stored data.
