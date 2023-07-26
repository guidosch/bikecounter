#ifndef LORACONNECTOR_H
#define LORACONNECTOR_H

#include <Arduino.h>
#include <MKRWAN.h>

class LoRaConnector
{
public:
    void setAppEui(String appEui) { eui = appEui; }
    void setAppKey(String appKey) { key = appKey; }

private:
    LoRaModem modem = LoRaModem(Serial1);
    String eui;
    String key;
};

#endif // LORACONNECTOR_H