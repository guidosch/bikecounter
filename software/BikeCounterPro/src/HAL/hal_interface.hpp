#ifndef HAL_H
#define HAL_H

#include <cstdint>
#include <string>

class HAL
{
public:
    typedef enum
    {
        INPUT = 0x0,
        OUTPUT = 0x1,
        INPUT_PULLUP = 0x2,
        INPUT_PULLDOWN = 0x3,
        OUTPUT_OPENDRAIN = 0x4,
    } GPIOPinMode;

    typedef enum
    {
        CHANGE = 2,
        FALLING = 3,
        RISING = 4,
    } TriggerMode;

    virtual void rtcBegin(bool resetTime = false) = 0;
    virtual void rtcSetEpoch(uint32_t ts) = 0;
    virtual uint32_t rtcGetEpoch() = 0;
    virtual uint8_t rtcGetHours() = 0;
    virtual uint8_t rtcGetMinutes() = 0;
    virtual uint8_t rtcGetSeconds() = 0;
    virtual uint8_t rtcGetDay() = 0;
    virtual uint8_t rtcGetMonth() = 0;
    virtual uint8_t rtcGetYear() = 0;

    virtual void I2CInit() = 0;
    virtual void AM2320Init() = 0;

    virtual float AM2320ReadTemperature() = 0;
    virtual float AM2320ReadHumidity() = 0;

    virtual bool getEuiAndKeyFromFlash(std::string *appEui, std::string *appKey) = 0;

    virtual unsigned long getMillis() = 0;
    virtual void waitHere(unsigned long ms) = 0;

    virtual int LoRaAvailable() = 0;
    virtual bool LoRaBegin() = 0;
    virtual std::string LoRaVersion() = 0;
    virtual std::string LoRaDeviceEUI() = 0;
    virtual int LoRaRead() = 0;
    virtual bool LoRaRestart() = 0;
    virtual int LoRaJoinOTAA(std::string eui, std::string key) = 0;
    virtual void LoRaSetMinPollInterval(unsigned long secs) = 0;
    virtual void LoRaBeginPacket() = 0;
    virtual size_t LoRaWrite(const uint8_t *msgBuffer, size_t msgSize) = 0;
    virtual int LoRaEndPacket(bool confirmed) = 0;

    virtual void SerialBeginAndWait(unsigned long baudrate) = 0;
    virtual size_t SerialPrintLn(std::string msg) = 0;

    virtual void digitalWrite(uint8_t pinNumber, unsigned int value) = 0;
    virtual int digitalRead(uint8_t pinNumber) = 0;
    virtual void pinMode(uint8_t pinNumber, GPIOPinMode pinMode) = 0;
    virtual void setAnalogReference() = 0;
    virtual void analogWrite(uint8_t pinNumber, int value) = 0;
    virtual int analogRead(uint8_t pinNumber) = 0;
    
    virtual void attachInterruptWakeup(uint32_t pin, void (*callback)(void), TriggerMode mode) = 0;
    virtual void deepSleep(int ms) = 0;
};

#endif // HAL_H