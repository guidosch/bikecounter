#include "statusLogger.hpp"

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

void StatusLogger::push(String &msg)
{
    std::string str{msg.c_str()};
    msgQueue.push(str);
}

void StatusLogger::push(const char *msg)
{
    std::string str{msg};
    msgQueue.push(str);
}