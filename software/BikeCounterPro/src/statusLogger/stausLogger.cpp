#include "statusLogger.hpp"

StatusLogger *StatusLogger::instance = nullptr;
// Thread-save Singleton (not needed for Arduino)
// std::mutex LoRaConnector::mutex_;

StatusLogger *StatusLogger::getInstance()
{
    // Thread-save Singleton (not needed for Arduino)
    // std::lock_guard<std::mutex> lock(mutex_);
    if (instance == nullptr)
    {
        instance = new StatusLogger();
    }
    return instance;
}

void StatusLogger::setup(Output ot)
{
    outputType = ot;

    switch (outputType)
    {
    case noOutput:
        currentStatus = ready;
        break;

    case toSerial:
        // open serial connection and wait
        Serial.begin(115200);
        while (!Serial)
            ;
        currentStatus = ready;
        break;

    case toMemory:
        // not implemented
        outputType = noOutput;
        break;

    default:
        outputType = noOutput;
        break;
    }
}

void StatusLogger::loop()
{
    if (currentStatus == ready && outputType != noOutput)
    {
        while (!msgQueue.empty())
        {
            std::string msg = msgQueue.front();

            switch (outputType)
            {
            case toSerial:
                Serial.println(msg.c_str());
                break;

            case toMemory:
                // not implemented
                break;
            }

            msgQueue.pop();
        }
    }
}

void StatusLogger::push(const std::string msg)
{
    msgQueue.push(msg);
}