#ifndef HAL_ARDUINO_H
#define HAL_ARDUINO_H

#include "hal.hpp"
#include <string>
#include <RTCZero.h>
#include <Adafruit_AM2320.h>
#include <ArduinoLowPower.h>
#include <SPI.h>
#include <SparkFun_SPI_SerialFlash.h>

class HAL_Arduino : public HAL
{
public:
    // Singletons should not be cloneable and not be assignable.
    HAL_Arduino(HAL_Arduino &other) = delete;
    void operator=(const HAL_Arduino &) = delete;

    static HAL_Arduino *getInstance();

    virtual void rtcBegin(bool resetTime = false) { rtc.begin(true); }
    virtual void rtcSetEpoch(uint32_t ts) { rtc.setEpoch(ts); }
    virtual uint32_t rtcGetEpoch() { return rtc.getEpoch(); }
    virtual uint8_t rtcGetHours() { return rtc.getHours(); }
    virtual uint8_t rtcGetMinutes() { return rtc.getMinutes(); }
    virtual uint8_t rtcGetSeconds() { return rtc.getSeconds(); }
    virtual uint8_t rtcGetDay() { return rtc.getDay(); }
    virtual uint8_t rtcGetMonth() { return rtc.getMonth(); }
    virtual uint8_t rtcGetYear() { return rtc.getYear(); }

    virtual void I2CInit() { Wire.begin(); }

    virtual void AM2320Init() { am2320.begin(); }
    virtual float AM2320ReadTemperature() { return am2320.readTemperature(); }
    virtual float AM2320ReadHumidity() { return am2320.readHumidity(); }

    virtual void attachInterruptWakeup(uint32_t pin, void (*callback)(void), int mode) { LowPower.attachInterruptWakeup(pin, callback, static_cast<PinStatus>(mode)); };
    virtual void deepSleep(int ms) { LowPower.deepSleep(ms); }
    
    virtual bool getEuiAndKeyFromFlash(std::string *appEui, std::string *appKey);
    
    virtual unsigned long getMillis() {return Arduino_h::millis();}

protected:
    HAL_Arduino() {}
    ~HAL_Arduino() {}

private:
    static HAL_Arduino *instance;
    // Thread-save Singleton (not needed for Arduino)
    // static std::mutex mutex_;

    // Internal RTC object
    RTCZero rtc;

    // Humidity and temperature sensor object
    Adafruit_AM2320 am2320 = Adafruit_AM2320();

    // SPI serial flash parameter
    const byte PIN_FLASH_CS = 32;
    // SPI serial flash object
    SFE_SPI_FLASH flash;
};

#endif // HAL_ARDUINO_H