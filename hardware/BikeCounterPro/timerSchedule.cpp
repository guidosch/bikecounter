#include "timerSchedule.hpp"

DateTime TimerSchedule::getNextIntervalTime(DateTime &cDT)
{
    int cId = getIntervalId(cDT.hour(), cDT.month());

    DateTime nDT = cDT + timeSpanIntervals[cId];
    // Check if next call is inside same day
    if ((nDT.day() != cDT.day()) && !isLastCall)
    {
        nDT = DateTime(cDT.year(), cDT.month(), cDT.day(), 23, 59, 59) - TimeSpan(0, 0, 10, 0);
        isLastCall = true;
    }
    else
    {
        isLastCall = false;

        // Check if next call is inside same interval
        // if not change the next call to the start of the new interval to sync the timing
        int nId = getIntervalId(nDT.hour(), nDT.month());
        if (nId != cId)
        {
            nDT = DateTime(cDT.year(), cDT.month(), cDT.day(), IntervalStartUTC[nId][nDT.month() - 1], 0, 0) + TimeSpan(0, 0, 1, 0);
        }
    }
};

int TimerSchedule::getIntervalId(int currentHour, int currentMonth)
{
    int intervalId = 0;
    for (int i = 0; i < intervalCount; ++i)
    {
        if (currentHour >= IntervalStartUTC[i][currentMonth - 1])
        {
            intervalId = i;
        }
    }
    return intervalId;
}
