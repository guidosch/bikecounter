#ifndef DATAPACKAGE_H
#define DATAPACKAGE_H

#include <math.h>

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
     */
    DataPackage(int intervalTime, int count = 0, int s = 0, int batLevel = 0, int temp = 0, int hum = 0, int hotd = 0);
    DataPackage(const DataPackage &co);
    ~DataPackage();
    // getter and setter
    void setMotionCount(int count) { motionCount = count; }
    int getMotionCount() const { return motionCount; }
    void setStatus(int s) { status = s; }
    int getStatus() const { return status; }
    void setBatteryLevel(int bl) { batteryLevel = bl; }
    int getBatteryLevel() { return batteryLevel; }
    void setTemperatur(int temp) { temperatur = temp; }
    int getTemperatur() { return temperatur; }
    void setHumidity(int hum) { humidity = hum; }
    int getHumidity() { return humidity; }
    void setHoureOfTheDay(int h) { houreOfTheDay = h; }
    int getHoureOfTheDay() { return houreOfTheDay; }
    // payload operations
    int getPayloadLength() { return 3 + 1 + (int)ceil(((float)(motionCount * minuteBits[selectedInterval])) / 8.0f); }
    byte *getPayload();

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

    unsigned int motionCount;
    unsigned int status;
    unsigned int batteryLevel;
    unsigned int temperatur;
    unsigned int humidity;
    unsigned int houreOfTheDay;

    byte payload[51] = {0};
};

#endif //DATAPACKAGE_H