#include "LoRaConnector.hpp"
#include <sstream>

void LoRaConnector::setup(String appEui, String appKey, StatusLogger &statusLogger, int (*downlinkCallbackFunction)(int *, int))
{
    eui = appEui;
    key = appKey;
    logger = statusLogger;
    downlinkCallback = downlinkCallbackFunction;

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
        // try to connect
        currentStatus = connecting;
        break;

    case connecting:
        int err = connectToNetwork();
        if (!err)
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
        if (sendRequested)
        {
            currentStatus = transmitting;
        }
        break;

    case transmitting: // uplink
        int err = sendData();
        if (!err)
        {
            currentStatus = error;
        }
        else
        {
            currentStatus = waiting;
            t = millis();
        }
        break;

    case waiting: // downlink
        if ((millis() - t) > downlinkTimeout)
        {
            if (modem.available())
            {
                currentStatus = reading;
            }
            else
            {
                logger.push("No downlink massage received.");
                logger.loop();

                currentStatus = connected;
            }
            sendRequested = 0;
        }
        break;

    case reading:
    {
        int rcv[64] = {0};
        int i = 0;
        while (modem.available())
        {
            rcv[i++] = modem.read();
        }

        std::ostringstream os("Received: ");
        for (unsigned int j = 0; j < i; j++)
        {
            os << (rcv[j] >> 4);
            os << (rcv[j] & 0xF);
            os << " ";
        }
        logger.push(os.str());
        logger.loop();

        // call the downlink callback function and pass the payload
        downlinkCallback(rcv, i);

        currentStatus = connected;
        break;
    }

    case error:
        logger.push(errorMsg[errorId]);
        logger.loop();
        ++errorCount;

        if (errorCount < 5)
        {
            currentStatus = disconnected;
        }
        else
        {
            currentStatus = fatalError;
        }
        break;

    case fatalError:
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

int LoRaConnector::sendMessage(const uint8_t *buffer, size_t size)
{
    if (sendRequested)
    {
        return 1;
    }
    if (currentStatus == connected)
    {
        return 2;
    }
    msgSize = size;
    for (int i = 0; i < msgSize; ++i)
    {
        msgBuffer[i] = buffer[i];
    }
    sendRequested = 1;
}

int LoRaConnector::sendData()
{
    modem.beginPacket();
    modem.write(msgBuffer, msgSize);
    int err = modem.endPacket(false);

    if (err > 0)
    {
        logger.push("Message sent correctly");
        logger.loop();
        return 0;
    }
    else
    {
        errorId = 3;
        return 1;
    }
}