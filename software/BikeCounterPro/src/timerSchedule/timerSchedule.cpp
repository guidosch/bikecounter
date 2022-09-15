#include "timerSchedule.hpp"

time_t TimerSchedule::getNextIntervalTime(time_t currentDateTime)
{
    tm *cDT = gmtime(&currentDateTime);
    int cId = getIntervalId(cDT->tm_hour, cDT->tm_mon);

    tm nDTr = *cDT;
    nDTr.tm_sec += timeSpanIntervals[cId];
    mktime(&nDTr);
    // Check if next call is inside same day
    if ((nDTr.tm_mday != cDT->tm_mday) && !isLastCall)
    {
        nDTr.tm_year = cDT->tm_year;
        nDTr.tm_mon = cDT->tm_mon;
        nDTr.tm_mday = cDT->tm_mday;
        nDTr.tm_hour = 23;
        nDTr.tm_min = 50;
        nDTr.tm_sec = 0;
        isLastCall = true;
    }
    else
    {
        isLastCall = false;

        // Check if next call is inside same interval
        // if not change the next call to the start of the new interval to sync the timing
        int nId = getIntervalId(nDTr.tm_hour, nDTr.tm_mon);
        if (nId != cId)
        {
            nDTr.tm_year = cDT->tm_year;
            nDTr.tm_mon = cDT->tm_mon;
            nDTr.tm_mday = cDT->tm_mday;
            nDTr.tm_hour = IntervalStartUTC[nId][nDTr.tm_mon];
            nDTr.tm_min = 1;
            nDTr.tm_sec = 0;
        }
    }
    return mktime(&nDTr) - _timezone;
};

uint32_t TimerSchedule::getCurrentIntervalSeconds(time_t cDT)
{
    tm *gmTm = gmtime(&cDT);
    int cId = getIntervalId(gmTm->tm_hour, gmTm->tm_mon);
    return timeSpanIntervals[cId];
};

uint32_t TimerSchedule::getCurrentIntervalMinutes(time_t cDT)
{
    return (uint32_t)(TimerSchedule::getCurrentIntervalSeconds(cDT) / 60);
};

int TimerSchedule::getIntervalId(int currentHour, int currentMonth)
{
    int intervalId = 0;
    for (int i = 0; i < intervalCount; ++i)
    {
        if (currentHour >= IntervalStartUTC[i][currentMonth])
        {
            intervalId = i;
        }
    }
    return intervalId;
}
