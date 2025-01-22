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

    if (!hal->LoRaBegin())
    {
        errorId = 1;
        currentStatus = error;
        return;
    };

    logger.push("Module version: " +
                hal->LoRaVersion());
    logger.push("Device EUI: " +
                hal->LoRaDeviceEUI());
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
                t = hal->getMillis();
            }
            break;
        }

        case waiting: // downlink
        {
            if ((hal->getMillis() - t) > downlinkTimeout)
            {
                if (hal->LoRaAvailable())
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
            while (hal->LoRaAvailable())
            {
                rcv[i++] = hal->LoRaRead();
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
    hal->LoRaRestart();
    currentStatus = disconnected;
}

/// @brief Tries to connect to the LoRa WAN network
/// @return error code
int LoRaConnector::connectToNetwork()
{
    logger.push("Connecting to network.");
    logger.loop();
    int join = hal->LoRaJoinOTAA(eui, key);
    if (!join)
    {
        return 1;
    }

    hal->LoRaSetMinPollInterval(120);
    // wait for all data transmission to finish
    hal->waitHere(500);
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
    hal->LoRaBeginPacket();
    hal->LoRaWrite(msgBuffer, msgSize);
    int err = hal->LoRaEndPacket(false);

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