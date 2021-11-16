#include "dataPackage.hpp"

DataPackage::DataPackage(int intervalTime,
                         int count = 0,
                         int s = 0,
                         int batLevel = 0,
                         int temp = 0,
                         int hum = 0,
                         int hotd = 0) : motionCount(count),
                                         status(s),
                                         batteryLevel(batLevel),
                                         temperatur(temp),
                                         humidity(hum),
                                         houreOfTheDay(hotd)
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

byte *DataPackage::getPayload()
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
    byte statusAndBat;
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
    byte tempAndHum;
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
    byte indexAndHOD
    
}