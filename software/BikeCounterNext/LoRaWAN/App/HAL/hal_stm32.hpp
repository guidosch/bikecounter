#ifndef HAL_STM32_H
#define HAL_STM32_H

#include "hal_interface.hpp"
#include <string>
// #include <mutex>  (not needed for single core MCU)

class HAL_STM32 : public HAL
{
public:
    // Singletons should not be cloneable and not be assignable.
    HAL_STM32(HAL_STM32 &other) = delete;
    void operator=(const HAL_STM32 &) = delete;

    static HAL_STM32 *getInstance();

    virtual void rtcBegin(bool resetTime = false) {}
    virtual void rtcSetEpoch(uint32_t ts) {}
    virtual uint32_t rtcGetEpoch() {}
    virtual uint8_t rtcGetHours() {}
    virtual uint8_t rtcGetMinutes() {}
    virtual uint8_t rtcGetSeconds() {}
    virtual uint8_t rtcGetDay() {}
    virtual uint8_t rtcGetMonth() {}
    virtual uint8_t rtcGetYear() {}

    virtual void I2CInit() {}

    virtual void AM2320Init() {}
    virtual float AM2320ReadTemperature() {}
    virtual float AM2320ReadHumidity() {}

    virtual bool getEuiAndKeyFromFlash(std::string *appEui, std::string *appKey);

    virtual unsigned long getMillis() {}
    virtual void waitHere(unsigned long ms) {};

    virtual int LoRaAvailable() {}
    virtual bool LoRaBegin() {}
    virtual std::string LoRaVersion() {}
    virtual std::string LoRaDeviceEUI() {}
    virtual int LoRaRead() {}
    virtual bool LoRaRestart() {}
    virtual int LoRaJoinOTAA(std::string eui, std::string key) {}
    virtual void LoRaSetMinPollInterval(unsigned long secs) {}
    virtual void LoRaBeginPacket() {}
    virtual size_t LoRaWrite(const uint8_t *msgBuffer, size_t msgSize) {}
    virtual int LoRaEndPacket(bool confirmed) {}

    virtual void SerialBeginAndWait(unsigned long baudrate) {}
    virtual size_t SerialPrintLn(std::string msg) {}

    virtual void digitalWrite(uint8_t pinNumber, unsigned int value) {}
    virtual int digitalRead(uint8_t pinNumber) {}
    virtual void pinMode(uint8_t pinNumber, GPIOPinMode pinMode) {}
    virtual void setAnalogReference() {}
    virtual void analogWrite(uint8_t pinNumber, int value) {}
    virtual int analogRead(uint8_t pinNumber) {}

    virtual void attachInterruptWakeup(uint32_t pin, void (*callback)(void), TriggerMode mode) {};
    virtual void deepSleep(int ms) {}

protected:
    HAL_STM32()
    {
    }
    ~HAL_STM32() {}

private:
    static HAL_STM32 *instance;
    // Thread-save Singleton
    // static std::mutex mutex_;
};

#endif // HAL_STM32_H
