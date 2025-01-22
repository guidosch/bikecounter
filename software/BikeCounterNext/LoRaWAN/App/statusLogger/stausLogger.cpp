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

void StatusLogger::setup(Output ot, HAL *hal_ptr)
{
    outputType = ot;
    hal = hal_ptr;

    switch (outputType)
    {
    case noOutput:
        currentStatus = ready;
        break;

    case toSerial:
        // open serial connection and wait
        hal->SerialBeginAndWait(115200);
        currentStatus = ready;
        break;

    case toMemory:
        // not implemented
        currentStatus = ready;
        break;

    default:
        outputType = noOutput;
        currentStatus = ready;
        break;
    }
}

void StatusLogger::loop()
{
    while (!msgQueue.empty())
    {
        std::string msg = msgQueue.front();
        msgQueue.pop();
        if (currentStatus == ready)
        {
            switch (outputType)
            {
            case toSerial:

                hal->SerialPrintLn(msg);
                break;

            case toMemory:
                // not implemented
                break;

            case noOutput:
                // no action needed
                break;
            }
        }
    }
}

void StatusLogger::push(const std::string msg)
{
    msgQueue.push(msg);
    
    // discard old messages if queue grows to fast
    if (msgQueue.size() > 20)
    {
        msgQueue.pop();
    }
}