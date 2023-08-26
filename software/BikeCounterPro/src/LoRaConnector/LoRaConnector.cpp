#include "LoRaConnector.hpp"

void LoRaConnector::setup(String appEui, String appKey, int debugFlag)
{
    eui = appEui;
    key = appKey;
    df = debugFlag;

    if (!modem.begin(EU868))
    {
        errorId = 1;
        currentStatus = error;
        return;
    };

    if (df)
    {
        Serial.print("Your module version is: ");
        Serial.println(modem.version());
        Serial.print("Your device EUI is: ");
        Serial.println(modem.deviceEUI());
    }
}

void LoRaConnector::loop()
{

    switch (currentStatus)
    {
    case disconnected:
        // try to connect

        currentStatus = connecting;
        break;
    case connecting:
        break;
    case connected: // and ready/idle
        break;
    case transmitting: // uplink
        break;
    case waiting: // downlink
        break;
    case error:
        if (!errorCount && df)
        {
            Serial.println(errorMsg[errorId]);
        }
        ++errorCount;

        break;
    }
};

// Connects to LoRa network
void doConnect()
{

    delay(4000); // increase up to 10s if connection does not work
    int connected = modem.joinOTAA(appEui, appKey);
    if (!connected)
    {
        if (debugFlag)
        {
            Serial.println("Something went wrong; are you indoor? Move near a window and retry");
        }
        while (1)
        {
            blinkLED();
        }
    }
    modem.minPollInterval(60);
    blinkLED(3);

    // wait for all data transmission to finish
    delay(500);
}