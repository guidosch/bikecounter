#ifndef DATAPACKAGE_H
#define DATAPACKAGE_H

#include <math.h>
#include <stdint.h>
#include "Arduino.h"

class DataPackage
{
public:
    /**
     * @brief Construct a new Data Package object
     * The DataPackage object handles the protocol encoding/decoding to send the lora data
     * @param intervalTime the timer interval in minutes
     * @param count 
     */

    /**
     * @brief Construct a new Data Package object
     * The DataPackage object handles the protocol encoding/decoding to send the lora data
     * @param intervalTime the timer interval in minutes
     * @param count (optional) motion counter value
     * @param s (optional) status
     * @param batLevel (optional) battery level
     * @param temp (optional) temperatur
     * @param hum (optional) humidity
     * @param hotd (optional) houre of the day
     * @param tVec (optional) pointer to the time array
     */
    DataPackage(unsigned int intervalTime,
                uint8_t count = 0,
                uint8_t s = 0,
                uint8_t batLevel = 0,
                uint8_t temp = 0,
                uint8_t hum = 0,
                uint8_t hotd = 0,
                uint8_t *tVec = nullptr);
    DataPackage(const DataPackage &co);
    ~DataPackage();
    // getter and setter
    void setMotionCount(uint8_t count) { motionCount = count; }
    uint8_t getMotionCount() const { return motionCount; }
    void setStatus(uint8_t s) { status = s; }
    uint8_t getStatus() const { return status; }
    void setBatteryLevel(uint8_t bl) { batteryLevel = bl; }
    void setBatteryLevel(float voltage);
    float getBatteryLevel() const;
    void setTemperatur(uint8_t temp) { temperatur = temp; }
    void setTemperatur(float temp);
    float getTemperatur() const;
    void setHumidity(uint8_t hum) { humidity = hum; }
    void setHumidity(float hum);
    float getHumidity() const;
    void setHoureOfTheDay(uint8_t h) { houreOfTheDay = h; }
    uint8_t getHoureOfTheDay() const { return houreOfTheDay; }
    void setTimeArray(uint8_t *arr) { timeVector = arr; }
    uint8_t *getTimeArray() const { return timeVector; }
    // payload operations
    int getPayloadLength() const { return 3 + 1 + (int)ceil(((float)(motionCount * minuteBits[selectedInterval])) / 8.0f); }
    uint8_t *getPayload();

private:
    enum TimerInterval
    {
        max_1h,
        max_2h,
        max_4h,
        max_8h,
        max_17h
    };
    TimerInterval selectedInterval = max_1h;
    int minuteBits[5] = {6, 7, 8, 9, 10};
    unsigned int bitCountBat = 5;
    unsigned int bitCountTemp = 5;
    unsigned int bitCountHum = 3;
    float minTemp = -20.0f;
    float maxTemp = 50.0f;

    uint8_t motionCount;
    uint8_t status;
    uint8_t batteryLevel;
    uint8_t temperatur;
    uint8_t humidity;
    uint8_t houreOfTheDay;
    uint8_t *timeVector;

    uint8_t payload[51] = {0};

    uint8_t reduceFloat(float value, float min, float max, unsigned int bitCount);
    float expandFloat(uint8_t value, float min, float max, unsigned int bitCount) const;
};

#endif //DATAPACKAGE_H