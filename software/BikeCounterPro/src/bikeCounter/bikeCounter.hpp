#ifndef BIKECOUNTER_H
#define BIKECOUNTER_H

#include <Arduino.h>
#include <RTCZero.h>
#include <ArduinoLowPower.h>
#include <Adafruit_AM2320.h>
#include <SPI.h>
#include <SparkFun_SPI_SerialFlash.h>
#include "../../config.h"
#include "../statusLogger/extendedStatusLogger.hpp"
#include "../LoRaConnector/LoRaConnector.hpp"
#include "../dataPackage/dataPackage.hpp"
#include "../timerSchedule/timerSchedule.hpp"
#include "../timerSchedule/date.h"

class BikeCounter
{
public:
    // Singletons should not be cloneable and not be assignable.
    BikeCounter(BikeCounter &other) = delete;
    void operator=(const BikeCounter &) = delete;

    static BikeCounter *getInstance();

    /// @brief
    enum Status
    {
        setupStep,
        initSleep,
        firstWakeUp,
        timeSync,
        collectData,
        sendPackage,
        waitForLoRa,
        sleepState,
        errorState
    };
    /// @brief
    void loop();
    /// @brief
    void reset();
    /// @brief
    /// @return
    Status getStatus() { return currentStatus; }
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
    /// @brief Sync time interval
    /// @param s seconds
    void setSyncTimeInterval(uint32_t s) { syncTimeInterval = s; }
    /// @brief Deactivate the onboard LED after the specified amount of blinks
    /// @param count
    void setMaxBlinks(int count) { maxBlinks = count; }
    /// @brief Max. counts between timer calls (to detect a floating interrupt pin)
    /// @param count
    void setMaxCount(int count) { maxCount = count; }
    /// @brief
    void correctRTCTime(int32_t timeDrift);

protected:
    BikeCounter() {}
    ~BikeCounter() {}

private:
    static BikeCounter *instance;
    // Thread-save Singleton (not needed for Arduino)
    // static std::mutex mutex_;

    int counterInterruptPin;
    int debugSwitchPin;
    int configSwitchPin;
    int batteryVoltagePin;
    int pirPowerPin;
    int usedPins[6] = {LED_BUILTIN, counterInterruptPin, debugSwitchPin, configSwitchPin, batteryVoltagePin, pirPowerPin};
    int usedPinCount = 6;
    uint32_t syncTimeInterval;
    int maxCount;
    int maxBlinks;

    // Object to log the status of the device
    ExtendedStatusLogger logger = ExtendedStatusLogger("BikeCounter:");

    // Object to handel all the LoRa stuff
    LoRaConnector *loRaConnector = LoRaConnector::getInstance();

    // Internal RTC object
    RTCZero rtc;

    // Humidity and temperature sensor object
    Adafruit_AM2320 am2320 = Adafruit_AM2320();

    // DataPackage object to encode the payload
    DataPackage dataHandler = DataPackage();

    // TimerSchedule object to determine the next timer call
    TimerSchedule timeHandler = TimerSchedule();

    // Motion counter value
    int counter = 0;
    // total counts between timer calls
    int totalCounter = 0;
    // motion detected flag (must be volatile as changed in IRS)
    volatile bool motionDetected;
    // time array size
    static const int timeArraySize = 62;
    // time array
    unsigned int timeArray[timeArraySize];
    // hour of the day for next package
    unsigned int hourOfDay = 0;
    // Error counter for pir-sensor
    int pirError = 0;
    // Holds the debug state of the dip switch
    int debugFlag = 0;
    // Holds the config state of the dip switch
    int configFlag = 0;
    // Last call of main loop in debug mode
    unsigned long lastMillis = millis() - 10 * 60 * 1000;
    // default startup date 01.01.2023 (1672531200)
    uint32_t defaultRTCEpoch = 1672531200ul;
    // Keep track of the last RTC correction (Prevents that the time correction is applied multiple times due to network lag and multiple enqueued downlinks with the same timeDrift information)
    uint32_t lastRTCCorrection = 0ul;
    // time sync state machine status
    unsigned timeSyncStat = 0;
    // SPI serial flash parameter
    const byte PIN_FLASH_CS = 32;
    // SPI serial flash object
    SFE_SPI_FLASH flash;
    // Next wakeup time (epoch)
    std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds> nextAlarm{std::chrono::seconds{0}};
    // current state machine state
    Status currentStatus = setupStep;
    // error code
    int errorId = 0;
    // error messages corresponding to the errorId
    char *errorMsg[5] = {"No error",
                         "SPI Flash not detected",
                         "Floating interrupt pin detected.",
                         "PIR sensor error",
                         "Error while sending message"};
    // recovered from error
    bool recErr = false;
    // sleep state variables
    Status preSleepStatus = setupStep;
    unsigned long sleepEndMillis = 0UL;

    /// @brief
    /// @return
    int setup();

    /// @brief
    /// @return
    int processInput();

    /// @brief
    /// @return 0=message sent correctly, 1=there was already a message in the queue, 2=error
    int sendUplinkMessage();

    /// @brief blinks the on-board led
    /// @param times number of times to blink
    void blinkLED(int times = 1);

    /// @brief Sets all the unused pins to a defined level (Output and LOW)
    void disableUnusedPins();

    /// @brief reads the analog value and calculates the battery voltage.
    /// @return Battery voltage
    float getBatteryVoltage();

    /// @brief
    /// @param ms
    void sleep(int ms, bool noInterrupt = false);

    /// @brief
    void handleError();

    /// @brief
    unsigned long getRemainingSleepTime(std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds> currentTime);

    /// @brief
    int waitForLoRaModule();

    /// @brief
    /// @param buffer
    /// @param length
    /// @return
    static int processDownlinkMessage(int *buffer, int length);

    /// @brief
    static void onMotionDetected()
    {
        BikeCounter::getInstance()->motionDetected = 1;
    }
};

#endif // BIKECOUNTER_H