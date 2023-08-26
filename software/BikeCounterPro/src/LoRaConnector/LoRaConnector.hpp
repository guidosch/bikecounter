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
        connected,    // and ready/idle
        transmitting, // uplink
        waiting,      // downlink
        error
    };
    Status getStatus() { return currentStatus; }
    int getErrorId() { return errorId; }
    void setAppEui(String appEui) { eui = appEui; }
    void setAppKey(String appKey) { key = appKey; }
    void setup(String appEui, String appKey, int debugFlag);
    void loop();
    // error messages corresponding to the errorId
    char *errorMsg[4] = {"No error",
                         "Failed to start module", 
                         "Geekfor"};

private:
    LoRaModem modem = LoRaModem(Serial1);
    Status currentStatus = disconnected;
    String eui;
    String key;
    int df = 0;
    int errorId = 0;
    int errorCount = 0;
};

#endif // LORACONNECTOR_H