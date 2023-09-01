#include "bikeCounter.hpp"

void BikeCounter::loop()
{
    switch (currentStatus)
    {
    case Status::setup:
        break;
    case Status::initSleep:
        break;
    case Status::connectToLoRa:
        break;
    case Status::collectData:
        break;
    case Status::sendPackage:
        break;
    case Status::waitForDownlink:
        break;
    case Status::adjustClock:
        break;
    case Status::error:

    default:
        break;
    }
}

void BikeCounter::reset()
{
}

void BikeCounter::setup()
{
}

void BikeCounter::setUsedPins(const int *const activePins, int size)
{
    usedPinCount = size;
    for (int i = 0; i < usedPinCount; ++i)
    {
        usedPins[i] = activePins[i];
    }
}

void BikeCounter::blinkLED(int times)
{
    // deactivate the onboard LED after the specified amount of blinks
    static int blinkCount = 0;
    if (blinkCount < maxBlinks)
    {
        ++blinkCount;
        for (int i = 0; i < times; i++)
        {
            delay(100);
            digitalWrite(LED_BUILTIN, HIGH);
            delay(100);
            digitalWrite(LED_BUILTIN, LOW);
        }
    }
}

void BikeCounter::disableUnusedPins()
{
    for (int i = 0; i < 22; ++i)
    {
        // check if the current pin occurs in the pin array
        bool isUsed = false;
        for (int j = 0; j < usedPinCount; ++j)
        {
            if (usedPins[j] == i)
            {
                isUsed = true;
            }
        }
        // if not, set the pin to output and low
        if (!isUsed)
        {
            pinMode(i, OUTPUT);
            digitalWrite(i, LOW);
        }
    }
}

float BikeCounter::getBatteryVoltage()
{
    // read the analog value and calculate the voltage
    return analogRead(batteryVoltagePin) * 3.3f / 1023.0f / 1.2f * (1.2f + 0.33f);
}