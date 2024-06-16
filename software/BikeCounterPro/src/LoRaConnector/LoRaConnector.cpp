#include "LoRaConnector.hpp"

LoRaConnector *LoRaConnector::instance = nullptr;
// Thread-save Singleton (not needed for Arduino)
// std::mutex LoRaConnector::mutex_;

LoRaConnector *LoRaConnector::getInstance()
{
    // Thread-save Singleton (not needed for Arduino)
    // std::lock_guard<std::mutex> lock(mutex_);
    if (instance == nullptr)
    {
        instance = new LoRaConnector();
    }
    return instance;
}

void LoRaConnector::setup(std::string appEui, std::string appKey, int (*downlinkCallbackFunction)(int *, int))
{
    eui = appEui;
    key = appKey;
    downlinkCallback = downlinkCallbackFunction;

    if (!modem.begin(EU868))
    {
        errorId = 1;
        currentStatus = error;
        return;
    };

    logger.push(String("Module version: ") +
                String(modem.version()));
    logger.push(String("Device EUI: ") +
                String(modem.deviceEUI()));
    logger.loop();
}

void LoRaConnector::loop(unsigned int times)
{
    for (int i = 0; i < times; ++i)
    {
        switch (currentStatus)
        {
        case disconnected:
            // try to connect
            currentStatus = connecting;
            break;

        case connecting:
        {
            int err = connectToNetwork();
            if (err)
            {
                errorId = 2;
                currentStatus = error;
            }
            else
            {
                currentStatus = connected;
            }
            break;
        }

        case connected: // and ready/idle
            if (sendRequested)
            {
                currentStatus = transmitting;
            }
            break;

        case transmitting: // uplink
        {
            int err = sendData();
            if (err)
            {
                currentStatus = error;
            }
            else
            {
                currentStatus = waiting;
                t = millis();
            }
            break;
        }

        case waiting: // downlink
        {
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
        }
        case reading:
        {
            int rcv[64] = {0};
            int i = 0;
            while (modem.available())
            {
                rcv[i++] = modem.read();
            }

            std::string os("Received ");
            for (unsigned int j = 0; j < i; j++)
            {
                os.append(std::to_string((rcv[j] >> 4)));
                os.append(std::to_string((rcv[j] & 0xF)));
                os.append(" ");
            }
            logger.push(os);
            logger.loop();

            // call the downlink callback function and pass the payload
            downlinkCallback(rcv, i);

            currentStatus = connected;
            break;
        }

        case error:
        {
            logger.push(errorMsg[errorId]);
            logger.loop();
            currentStatus = disconnected;
            break;
        }

        case fatalError:
            break;
        }
    }
};

void LoRaConnector::reset()
{
    modem.restart();
    currentStatus = disconnected;
}

/// @brief Tries to connect to the LoRa WAN network
/// @return error code
int LoRaConnector::connectToNetwork()
{
    logger.push("Connecting to network.");
    logger.loop();
    int join = modem.joinOTAA(eui.c_str(), key.c_str());
    if (!join)
    {
        return 1;
    }

    modem.minPollInterval(120);
    // wait for all data transmission to finish
    delay(500);
    logger.push("Successfully connected to network.");
    logger.loop();
    return 0;
}

int LoRaConnector::sendMessage(const uint8_t *buffer, size_t size)
{
    if (sendRequested)
    {
        return 1;
    }
    if (currentStatus != connected)
    {
        return 2;
    }
    msgSize = size;
    for (int i = 0; i < msgSize; ++i)
    {
        msgBuffer[i] = buffer[i];
    }
    sendRequested = 1;
    logger.push("Message enqueued");
    logger.loop();
    return 0;
}

int LoRaConnector::sendData()
{
    logger.push("Message transmission started");
    logger.loop();
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