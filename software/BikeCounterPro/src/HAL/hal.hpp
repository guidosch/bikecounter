#ifndef HAL_H
#define HAL_H

#include <cstdint>

class HAL
{
public:
    virtual void rtcBegin(bool resetTime = false) = 0;
    virtual void rtcSetEpoch(uint32_t ts) = 0;
    virtual uint32_t rtcGetEpoch() = 0;
    virtual uint8_t rtcGetHours() = 0;
    virtual uint8_t rtcGetMinutes() = 0;
    virtual uint8_t rtcGetSeconds() = 0;
    virtual uint8_t rtcGetDay() = 0;
    virtual uint8_t rtcGetMonth() = 0;
    virtual uint8_t rtcGetYear() = 0;
};

#endif // HAL_H