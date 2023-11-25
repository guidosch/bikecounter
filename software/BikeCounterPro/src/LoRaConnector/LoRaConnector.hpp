#ifndef LORACONNECTOR_H
#define LORACONNECTOR_H

#include <Arduino.h>
#include <MKRWAN.h>
#include "../statusLogger/extendedStatusLogger.hpp"

class LoRaConnector
{
public:
    // Singletons should not be cloneable and not be assignable.
    LoRaConnector(LoRaConnector &other) = delete;
    void operator=(const LoRaConnector &) = delete;

    static LoRaConnector *getInstance();

    enum Status
    {
        disconnected,
        connecting,
        connected,    // and ready/idle
        transmitting, // uplink
        waiting,      // downlink
        reading,      // downlink
        error,
        fatalError
    };
    Status getStatus() { return currentStatus; }
    int getErrorId() { return errorId; }
    String getErrorMsg() { return String(errorMsg[errorId]); }
    void setAppEui(String appEui) { eui = appEui; }
    void setAppKey(String appKey) { key = appKey; }
    void setup(String appEui, String appKey, int (*downlinkCallbackFunction)(int *, int));
    void loop(unsigned int times = 1u);
    void reset();
    /// @brief
    /// @param buffer
    /// @param size
    /// @return 0 = message enqueued and ready to send;
    ///         1 = already another message enqueued (only one message can be sent at the time)
    ///         2 = error
    int sendMessage(const uint8_t *buffer, size_t size);

protected:
    LoRaConnector() {}
    ~LoRaConnector() {}

private:
    static LoRaConnector *instance;
    // Thread-save Singleton (not needed for Arduino)
    // static std::mutex mutex_;

    ExtendedStatusLogger logger = ExtendedStatusLogger("LoRaConnector:");
    LoRaModem modem = LoRaModem(Serial1);
    Status currentStatus = disconnected;
    String eui;
    String key;
    int errorId = 0;
    int errorCount = 0;
    int sendRequested = 0;
    uint8_t msgBuffer[51] = {0};
    size_t msgSize = 0;
    int connectToNetwork();
    int sendData();
    unsigned long t;
    unsigned long downlinkTimeout = 10000;
    int (*downlinkCallback)(int *, int);
    // error messages corresponding to the errorId
    char *errorMsg[4] = {"No error",
                         "Failed to start module",
                         "Failed to connect to LoRa network",
                         "Error sending message"};
};

#endif // LORACONNECTOR_H