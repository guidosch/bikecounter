#include "LoRaConnector.hpp"

void LoRaConnector::setup(String appEui, String appKey, StatusLogger &statusLogger)
{
    eui = appEui;
    key = appKey;
    logger = statusLogger;

    if (!modem.begin(EU868))
    {
        errorId = 1;
        currentStatus = error;
        return;
    };

    logger.push(String("Your module version is: ") +
                String(modem.version()));
    logger.push(String("Your device EUI is: ") +
                String(modem.deviceEUI()));
    logger.loop();
}

void LoRaConnector::loop()
{

    switch (currentStatus)
    {
    case disconnected:
        if (errorCount < 5)
        {
            // try to connect
            currentStatus = connecting;
        }
        break;
    case connecting:
        int stat = connectToNetwork();
        if (!stat)
        {
            errorId = 2;
            currentStatus = error;
        }
        else
        {
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
        logger.push(errorMsg[errorId]);
        logger.loop();
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