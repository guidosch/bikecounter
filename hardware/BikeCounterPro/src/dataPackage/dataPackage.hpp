#ifndef DATAPACKAGE_H
#define DATAPACKAGE_H

#include <math.h>
#include <stdint.h>
#include <Arduino.h>

class DataPackage
{
public:
    /**
     * @brief Construct a new Data Package object
     * The DataPackage object handles the protocol encoding/decoding to send the lora data
     */
    DataPackage(){}; 
    /**
     * @brief Construct a new Data Package object
     * The DataPackage object handles the protocol encoding/decoding to send the lora data
     * @param intervalTime the timer interval in minutes
     * @param count (optional) motion counter value
     * @param s (optional) status
     * @param batVoltage (optional) battery voltage
     * @param temp (optional) temperature
     * @param hum (optional) humidity
     * @param hotd (optional) hour of the day
     * @param tVec (optional) pointer to the time array
     */
    DataPackage(unsigned int intervalTime,
                uint8_t count = 0,
                uint8_t s = 0,
                uint8_t hwV = 0,
                uint8_t swV = 0,
                uint8_t batVoltage = 0,
                uint8_t temp = 0,
                uint8_t hum = 0,
                uint8_t hotd = 0,
                unsigned int *tVec = nullptr);
    DataPackage(const DataPackage &co);
    // getter and setter
    void setMotionCount(uint8_t count) { motionCount = count; }
    uint8_t getMotionCount() const { return motionCount; }
    void setStatus(uint8_t s) { status = s; }
    uint8_t getStatus() const { return status; }
    void setSwVersion(uint8_t v) { swVersion = v; }
    uint8_t getSwVersion() const { return swVersion; }
    void setHwVersion(uint8_t v) { hwVersion = v; }
    uint8_t getHwVersion() const { return hwVersion; }
    void setBatteryVoltage(uint8_t bl) { batteryVoltage = bl; }
    void setBatteryVoltage(float voltage);
    float getBatteryVoltage() const;
    void setTemperature(uint8_t temp) { temperature = temp; }
    void setTemperature(float temp);
    float getTemperature() const;
    void setHumidity(uint8_t hum) { humidity = hum; }
    void setHumidity(float hum);
    float getHumidity() const;
    void setHoureOfTheDay(uint8_t h) { houreOfTheDay = h; }
    uint8_t getHoureOfTheDay() const { return houreOfTheDay; }
    void setTimeArray(unsigned int *arr) { timeVector = arr; }
    unsigned int *getTimeArray() const { return timeVector; }
    // payload operations
    int getPayloadLength() const { return 3 + 1 + (int)ceil(((float)(motionCount * minuteBits[selectedInterval])) / 8.0f); }
    uint8_t *getPayload();
    int getMaxCount(unsigned int intervalTime);

private:
    enum TimerInterval
    {
        max_1h,
        max_2h,
        max_4h,
        max_8h,
        max_17h
    };
    int maxCount[5] = {61, 52, 46, 40, 36};
    TimerInterval selectedInterval = max_1h;
    int minuteBits[5] = {6, 7, 8, 9, 10};
    unsigned int bitCountBat = 5;
    unsigned int bitCountTemp = 5;
    unsigned int bitCountHum = 3;
    float minTemp = -20.0f;
    float maxTemp = 50.0f;
    float minVoltage = 3.0f;
    float maxVoltage = 4.5f;

    uint8_t motionCount;
    uint8_t status;
    uint8_t swVersion;
    uint8_t hwVersion;
    uint8_t batteryVoltage;
    uint8_t temperature;
    uint8_t humidity;
    uint8_t houreOfTheDay;
    unsigned int *timeVector;

    uint8_t payload[51] = {0};

    uint8_t reduceFloat(float value, float min, float max, unsigned int bitCount);
    float expandFloat(uint8_t value, float min, float max, unsigned int bitCount) const;
    void setTimerInterval(unsigned int intervalTime);
};

#endif // DATAPACKAGE_H