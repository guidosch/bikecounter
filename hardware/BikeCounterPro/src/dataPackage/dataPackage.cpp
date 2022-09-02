#include "dataPackage.hpp"

#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitValue) (bitValue ? bitSet(value, bit) : bitClear(value, bit))
#define bitRead(value, bit) (((value) >> (bit)) & 0x01)

DataPackage::DataPackage(unsigned int intervalTime,
                         uint8_t count,
                         uint8_t s,
                         uint8_t hwV,
                         uint8_t swV,
                         uint8_t batVoltage,
                         uint8_t temp,
                         uint8_t hum,
                         uint8_t hotd,
                         uint32_t t,
                         unsigned int *tVec) : motionCount(count),
                                               status(s),
                                               hwVersion(hwV),
                                               swVersion(swV),
                                               batteryVoltage(batVoltage),
                                               temperature(temp),
                                               humidity(hum),
                                               hourOfTheDay(hotd),
                                               deviceTime(t),
                                               timeVector(tVec)
{
    setTimerInterval(intervalTime);
}

void DataPackage::setTimerInterval(unsigned int intervalTime)
{
    // set the interval index according to the selected timer interval
    if (intervalTime <= 60)
    {
        selectedInterval = max_1h;
    }
    else if (intervalTime <= 120)
    {
        selectedInterval = max_2h;
    }
    else if (intervalTime <= 240)
    {
        selectedInterval = max_4h;
    }
    else if (intervalTime <= 480)
    {
        selectedInterval = max_8h;
    }
    else if (intervalTime <= 1020)
    {
        selectedInterval = max_17h;
    }
    else
    {
        selectedInterval = max_17h;
    }
}

uint8_t *DataPackage::getPayload()
{
    // reset array to avoid sending old data
    // this should not happen due to the payload length property (getPayloadLength()) but you never know
    for (int i = 0; i < 51; ++i)
    {
        payload[i] = 0;
    }

    // 1. byte - counter value
    payload[0] = (uint8_t)(motionCount & 0xff);

    // 2. byte - software and hardware version
    uint8_t swAndHwVersion;
    bitWrite(swAndHwVersion, 0, bitRead(swVersion, 0));
    bitWrite(swAndHwVersion, 1, bitRead(swVersion, 1));
    bitWrite(swAndHwVersion, 2, bitRead(swVersion, 2));
    bitWrite(swAndHwVersion, 3, bitRead(swVersion, 3));
    bitWrite(swAndHwVersion, 4, bitRead(hwVersion, 0));
    bitWrite(swAndHwVersion, 5, bitRead(hwVersion, 1));
    bitWrite(swAndHwVersion, 6, bitRead(hwVersion, 2));
    bitWrite(swAndHwVersion, 7, bitRead(hwVersion, 3));
    payload[1] = swAndHwVersion;

    // 3. byte - status and battery Voltage
    uint8_t statusAndBat;
    bitWrite(statusAndBat, 0, bitRead(status, 0));
    bitWrite(statusAndBat, 1, bitRead(status, 1));
    bitWrite(statusAndBat, 2, bitRead(status, 2));
    bitWrite(statusAndBat, 3, bitRead(batteryVoltage, 0));
    bitWrite(statusAndBat, 4, bitRead(batteryVoltage, 1));
    bitWrite(statusAndBat, 5, bitRead(batteryVoltage, 2));
    bitWrite(statusAndBat, 6, bitRead(batteryVoltage, 3));
    bitWrite(statusAndBat, 7, bitRead(batteryVoltage, 4));
    payload[2] = statusAndBat;

    // 4. byte - temperature and humidity
    uint8_t tempAndHum;
    bitWrite(tempAndHum, 0, bitRead(temperature, 0));
    bitWrite(tempAndHum, 1, bitRead(temperature, 1));
    bitWrite(tempAndHum, 2, bitRead(temperature, 2));
    bitWrite(tempAndHum, 3, bitRead(temperature, 3));
    bitWrite(tempAndHum, 4, bitRead(temperature, 4));
    bitWrite(tempAndHum, 5, bitRead(humidity, 0));
    bitWrite(tempAndHum, 6, bitRead(humidity, 1));
    bitWrite(tempAndHum, 7, bitRead(humidity, 2));
    payload[3] = tempAndHum;

    // 5. byte - interval index and hour of the day
    uint8_t indexAndHOD;
    bitWrite(indexAndHOD, 0, bitRead(selectedInterval, 0));
    bitWrite(indexAndHOD, 1, bitRead(selectedInterval, 1));
    bitWrite(indexAndHOD, 2, bitRead(selectedInterval, 2));
    bitWrite(indexAndHOD, 3, bitRead(hourOfTheDay, 0));
    bitWrite(indexAndHOD, 4, bitRead(hourOfTheDay, 1));
    bitWrite(indexAndHOD, 5, bitRead(hourOfTheDay, 2));
    bitWrite(indexAndHOD, 6, bitRead(hourOfTheDay, 3));
    bitWrite(indexAndHOD, 7, bitRead(hourOfTheDay, 4));
    payload[4] = indexAndHOD;

    // 6. - 8. byte - device time
    uint32_t deviceTimeMinutes = deviceTime / 60;
    payload[5] = (uint8_t)(deviceTimeMinutes & 0xff);
    payload[6] = (uint8_t)((deviceTimeMinutes >> 8) & 0xff);
    payload[7] = (uint8_t)((deviceTimeMinutes >> 16) & 0xff);

    // 9. - 51. byte - detected minutes
    unsigned int offsetBits = 8 * 8;
    for (int payloadBit = offsetBits; payloadBit < ((motionCount * minuteBits[selectedInterval]) + offsetBits); ++payloadBit)
    {
        unsigned int currentMotionByte = (payloadBit - offsetBits) / minuteBits[selectedInterval];
        unsigned int currentMotionBit = (payloadBit - offsetBits) % minuteBits[selectedInterval];
        unsigned int currentPayloadByte = payloadBit / 8;
        unsigned int currentPayloadBit = payloadBit % 8;

        bitWrite(payload[currentPayloadByte], currentPayloadBit, bitRead(timeVector[currentMotionByte], currentMotionBit));
    }

    return payload;
}

uint8_t DataPackage::reduceFloat(float value, float min, float max, unsigned int bitCount)
{
    if (value < min)
    {
        value = min;
    }
    if (value > max)
    {
        value = max;
    }
    float dy = ((unsigned int)1) << bitCount;
    float dx = max - min;
    float slope = dy / dx;
    return (uint8_t)(round(slope * (value - min)));
}

float DataPackage::expandFloat(uint8_t value, float min, float max, unsigned int bitCount) const
{
    float dy = (((unsigned int)1) << bitCount) - 1;
    float dx = max - min;
    float slope = dy / dx;
    return (((float)value) / slope + ((float)min));
}

void DataPackage::setBatteryVoltage(float voltage)
{
    batteryVoltage = reduceFloat(voltage, minVoltage, maxVoltage, bitCountBat);
}

float DataPackage::getBatteryVoltage() const
{
    return expandFloat(batteryVoltage, minVoltage, maxVoltage, bitCountBat);
}

void DataPackage::setTemperature(float temp)
{
    temperature = reduceFloat(temp, minTemp, maxTemp, bitCountTemp);
}

float DataPackage::getTemperature() const
{
    return expandFloat(temperature, minTemp, maxTemp, bitCountTemp);
}

void DataPackage::setHumidity(float hum)
{
    humidity = reduceFloat(hum, 0, 100, bitCountHum);
}
float DataPackage::getHumidity() const
{
    return expandFloat(humidity, 0, 100, bitCountHum);
}

int DataPackage::getMaxCount(unsigned int intervalTime)
{
    setTimerInterval(intervalTime);
    return maxCount[selectedInterval];
}