#include "bikeCounter.hpp"

void BikeCounter::loop()
{
    switch (currentStatus)
    {
    case setup:
        break;
    case initSleep:
        break;
    case connectToLoRa:
        break;
    case collectData:
        break;
    case sendPackage:
        break;
    case waitForDownlink:
        break;
    case adjustClock:
        break;
    case error:

    default:
        break;
    }
}

void BikeCounter::reset()
{
}
