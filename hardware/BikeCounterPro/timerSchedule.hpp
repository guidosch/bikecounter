#ifndef TIMERSCHEDULE_H
#define TIMERSCHEDULE_H

#include "RTClib.h"

class TimerSchedule
{
public:
    TimerSchedule();
    TimeSpan getCurrentTimerIntervall(DateTime currentDateTime);

private:
    TimeSpan shortInterval = TimeSpan(0, 2, 0, 0);
    TimeSpan longInterval = TimeSpan(0, 6, 0, 0);
    int startHourShortInterval [12] = { 8,  7,  6,  6,  6,  6,  6,  6,  7,  7,  7,  8};
    int startHourLongtInterval [12] = {17, 18, 20, 21, 22, 22, 22, 21, 20, 19, 17, 17};
    bool isLastInterval(DateTime currentDateTime);
};

#endif //TIMERSCHEDULE_H