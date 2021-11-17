#include "dataPackage.hpp"

DataPackage::DataPackage(unsigned int intervalTime,
                         uint8_t count,
                         uint8_t s,
                         uint8_t batLevel,
                         uint8_t temp,
                         uint8_t hum,
                         uint8_t hotd,
                         uint8_t *tVec) : motionCount(count),
                                          status(s),
                                          batteryLevel(batLevel),
                                          temperatur(temp),
                                          humidity(hum),
                                          houreOfTheDay(hotd),
                                          timeVector(tVec)
{
    //set the interval index according to the selected timer interval
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
    //reset array to avoid sending old data
    //this should not happen due to the payload length property (getPayloadLength()) but you never know
    for (int i = 0; i < 51; ++i)
    {
        payload[i] = 0;
    }

    // 1. byte - counter value
    payload[0] = lowByte(motionCount);

    // 2. byte - status and battery level
    uint8_t statusAndBat;
    bitWrite(statusAndBat, 0, bitRead(status, 0));
    bitWrite(statusAndBat, 1, bitRead(status, 1));
    bitWrite(statusAndBat, 2, bitRead(status, 2));
    bitWrite(statusAndBat, 3, bitRead(batteryLevel, 0));
    bitWrite(statusAndBat, 4, bitRead(batteryLevel, 1));
    bitWrite(statusAndBat, 5, bitRead(batteryLevel, 2));
    bitWrite(statusAndBat, 6, bitRead(batteryLevel, 3));
    bitWrite(statusAndBat, 7, bitRead(batteryLevel, 4));
    payload[1] = statusAndBat;

    // 3. byte - temperatur and humidity
    uint8_t tempAndHum;
    bitWrite(tempAndHum, 0, bitRead(temperatur, 0));
    bitWrite(tempAndHum, 1, bitRead(temperatur, 1));
    bitWrite(tempAndHum, 2, bitRead(temperatur, 2));
    bitWrite(tempAndHum, 3, bitRead(temperatur, 3));
    bitWrite(tempAndHum, 4, bitRead(temperatur, 4));
    bitWrite(tempAndHum, 5, bitRead(humidity, 0));
    bitWrite(tempAndHum, 6, bitRead(humidity, 1));
    bitWrite(tempAndHum, 7, bitRead(humidity, 2));
    payload[2] = tempAndHum;

    // 4. byte - intervall index and hour of the day
    uint8_t indexAndHOD;
    bitWrite(indexAndHOD, 0, bitRead(selectedInterval, 0));
    bitWrite(indexAndHOD, 1, bitRead(selectedInterval, 1));
    bitWrite(indexAndHOD, 2, bitRead(selectedInterval, 2));
    bitWrite(indexAndHOD, 3, bitRead(houreOfTheDay, 0));
    bitWrite(indexAndHOD, 4, bitRead(houreOfTheDay, 1));
    bitWrite(indexAndHOD, 5, bitRead(houreOfTheDay, 2));
    bitWrite(indexAndHOD, 6, bitRead(houreOfTheDay, 3));
    bitWrite(indexAndHOD, 7, bitRead(houreOfTheDay, 4));
    payload[3] = indexAndHOD;

    // 5. - 51. byte - detected minutes
    unsigned int offsetBits = 4 * 8;
    for (int payloadBit = offsetBits; payloadBit < ((motionCount * minuteBits[selectedInterval]) + offsetBits); ++payloadBit)
    {
        unsigned int currentMotionByte = payloadBit / minuteBits[selectedInterval];
        unsigned int currentMotionBit = payloadBit % minuteBits[selectedInterval];
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

void DataPackage::setBatteryLevel(float voltage)
{
    // charch calculation
    // devided into two ranges >=3.8V and <3.8
    // curve parameters from pseudo invers matrix polynom
    float batLf = 0;
    if (voltage >= 3.8f)
    {
        batLf = -178.57f * voltage * voltage + 1569.6f * voltage - 3342.0f;
    }
    else
    {
        batLf = 1183.7f * voltage * voltage * voltage * voltage - 15843.0f * voltage * voltage * voltage + 79461.0f * voltage * voltage - 177004.0f * voltage + 147744.0f;
    }
    batteryLevel = reduceFloat(batLf, 0, 100, bitCountBat);
}

float DataPackage::getBatteryLevel() const
{
    return expandFloat(batteryLevel, 0, 100, bitCountBat);
}

void DataPackage::setTemperatur(float temp)
{
    temperatur = reduceFloat(temp, minTemp, maxTemp, bitCountTemp);
}

float DataPackage::getTemperatur() const
{
    return expandFloat(temperatur, minTemp, maxTemp, bitCountTemp);
}

void DataPackage::setHumidity(float hum)
{
    humidity = reduceFloat(hum, 0, 100, bitCountHum);
}
float DataPackage::getHumidity() const
{
    return expandFloat(humidity, 0, 100, bitCountHum);
}