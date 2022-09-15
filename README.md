# BikeCounter

This repository contains the hardware and software components of a [PIR](https://en.wikipedia.org/wiki/Passive_infrared_sensor) based tracking device to monitor the usage of local bike trails. The data is sent over [LoraWAN](https://de.wikipedia.org/wiki/Long_Range_Wide_Area_Network) to [TTN](https://www.thethingsnetwork.org/) and from there to a [Google cloud](https://console.cloud.google.com) backend which stores the data and provides api endpoints for the data visualization web UI ([bikeCounterUI](https://github.com/guidosch/bikecounterUI)).
