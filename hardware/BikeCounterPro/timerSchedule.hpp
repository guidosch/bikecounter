#ifndef TIMERSCHEDULE_H
#define TIMERSCHEDULE_H

#include "RTClib.h"

class TimerSchedule
{
public:
    TimerSchedule();
    DateTime getNextIntervalTime(DateTime &currentDateTime);

private:
    static const int intervalCount = 3;
    TimeSpan timeSpanIntervals[intervalCount] = {TimeSpan(0, 6, 0, 0), TimeSpan(0, 2, 0, 0), TimeSpan(0, 6, 0, 0)}; // {dayInterval, nightInterval}
    int IntervalStartUTC[intervalCount][12] = {
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {7, 6, 5, 5, 5, 5, 5, 5, 6, 6, 6, 7},              // ZRH (+1) { 8,  7,  6,  6,  6,  6,  6,  6,  7,  7,  7,  8} start day interval
        {16, 17, 19, 20, 21, 21, 21, 20, 19, 18, 16, 16}}; // ZRH (+1) {17, 18, 20, 21, 22, 22, 22, 21, 20, 19, 17, 17} start night interval
    bool isLastCall = false;
    int getIntervalId(int currentHour, int currentMonth);
};

#endif //TIMERSCHEDULE_H