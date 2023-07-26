#ifndef BIKECOUNTER_H
#define BIKECOUNTER_H

class BikeCounter
{
public:
    enum Status
    {
        setup,
        initSleep,
        connectToLoRa,
        collectData,
        sendPackage,
        waitForDownlink,
        adjustClock,
        error = 100

    };
    void loop();
    void reset();
    BikeCounter::Status getStatus() { return currentStatus; }

private:
    BikeCounter::Status currentStatus = setup;
};

#endif // BIKECOUNTER_H