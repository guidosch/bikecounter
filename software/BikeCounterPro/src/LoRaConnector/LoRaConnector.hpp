#ifndef LORACONNECTOR_H
#define LORACONNECTOR_H

#include <Arduino.h>
#include <MKRWAN.h>

class LoRaConnector
{
public:
    enum Status
    {
        disconnected,
        connecting,
        connected
    };
    Status getStatus() { return currentStatus; }
    void setAppEui(String appEui) { eui = appEui; }
    void setAppKey(String appKey) { key = appKey; }
    void loop();
    void connect();

private:
    LoRaModem modem = LoRaModem(Serial1);
    Status currentStatus = disconnected;
    String eui;
    String key;
};

#endif // LORACONNECTOR_H