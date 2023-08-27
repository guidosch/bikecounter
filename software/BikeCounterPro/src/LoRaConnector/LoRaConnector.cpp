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
        if (errorCount < 5){
            // try to connect
            currentStatus = connecting;
        }        
        break;
    case connecting:
        int stat = connectToNetwork();
        if (!stat){
            errorId = 2;
            currentStatus = error;
        }
        else{
            currentStatus = connected;
        }
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

/// @brief Tries to connect to the LoRa WAN network
/// @return error code
int LoRaConnector::connectToNetwork()
{
    int status = modem.joinOTAA(eui, key);
    if (!status)
    {
        return status;
    }

    modem.minPollInterval(120);
    // wait for all data transmission to finish
    delay(500);
    return 0;
}