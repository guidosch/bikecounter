#ifndef HAL_ARDUINO_H
#define HAL_ARDUINO_H

#include "hal.hpp"
#include <RTCZero.h>

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

protected:
    HAL_Arduino() {}
    ~HAL_Arduino() {}

private:
    static HAL_Arduino *instance;
    // Thread-save Singleton (not needed for Arduino)
    // static std::mutex mutex_;

    // Internal RTC object
    RTCZero rtc;
};

#endif // HAL_ARDUINO_H