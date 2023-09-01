#ifndef BIKECOUNTER_H
#define BIKECOUNTER_H

class BikeCounter
{
public:
    /// @brief
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
    /// @brief
    void loop();
    /// @brief
    void reset();
    /// @brief
    /// @return
    BikeCounter::Status getStatus() { return currentStatus; }
    /// @brief Counter interrupt pin
    /// @param pin
    void setCounterInterruptPin(int pin) { counterInterruptPin = pin; }
    /// @brief Debug switch pin
    /// @param pin
    void setDebugSwitchPin(int pin) { debugSwitchPin = pin; }
    /// @brief Config switch pin
    /// @param pin
    void setConfigSwitchPin(int pin) { configSwitchPin = pin; }
    /// @brief Analog input pin for battery voltage measurement
    /// @param pin
    void setBatteryVoltagePin(int pin) { batteryVoltagePin = pin; }
    /// @brief PIR power output pin (not implemented in PCB v0.1)
    /// @param pin
    void setPirPowerPin(int pin) { pirPowerPin = pin; }
    /// @brief Define active pins (not defined pins will be disabled to save power)
    /// @param activePins
    /// @param size
    void setUsedPins(const int *const activePins, int size);
    /// @brief Debug sleep interval
    /// @param ms milliseconds
    void setDebugSleepTime(uint32_t ms) { debugSleepTime = ms; }
    /// @brief Sync time interval
    /// @param ms milliseconds
    void setSyncTimeInterval(uint32_t ms) { syncTimeInterval = ms; }
    /// @brief Deactivate the onboard LED after the specified amount of blinks
    /// @param count
    void setMaxBlink(int count) { maxBlinks = count; }
    /// @brief Max. counts between timer calls (to detect a floating interrupt pin)
    /// @param count
    void setMaxCount(int count) { maxCount = count; }

private:
    int counterInterruptPin;
    int debugSwitchPin;
    int configSwitchPin;
    int batteryVoltagePin;
    int pirPowerPin;
    int usedPins[64];
    int usedPinCount;
    uint32_t debugSleepTime;
    uint32_t syncTimeInterval;
    int maxCount;
    int maxBlinks;

    BikeCounter::Status currentStatus = BikeCounter::Status::setup;
    void setup();
    /// @brief blinks the on-board led
    /// @param times number of times to blink
    void blinkLED(int times);
    /// @brief Sets all the unused pins to a defined level (Output and LOW)
    void disableUnusedPins();
    /// @brief reads the analog value and calculates the battery voltage.
    /// @return Battery voltage
    float BikeCounter::getBatteryVoltage();
};

#endif // BIKECOUNTER_H