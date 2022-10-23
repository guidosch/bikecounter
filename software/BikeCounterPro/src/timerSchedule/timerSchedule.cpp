#include "timerSchedule.hpp"

#ifdef __linux__
#define TIMEZONE timezone;
#else
#define TIMEZONE _timezone;
#endif

time_t TimerSchedule::getNextIntervalTime(time_t currentDateTime)
{
    tm *cDT = gmtime(&currentDateTime);
    int cId = getIntervalId(cDT->tm_hour, cDT->tm_mon);

    tm nDTr = *cDT;
    nDTr.tm_sec += timeSpanIntervals[cId];
    mktimeutc(&nDTr);
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
    return mktimeutc(&nDTr);
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

time_t mktimeutc(tm *timeptr)
{
    // apply the inverse procedure as in ANSI mktime to remove the local time attributes.

    // remove timezone
    unsigned long seconds = mktime(timeptr);
    seconds -= TIMEZONE;

    // remove day light saving
    unsigned dst;
    if (timeptr->tm_isdst > 0)
    {
        // day light saving was applied
        dst = 60 * 60;
    }
    else if (timeptr->tm_isdst == 0)
    {
        // no day light saving was applied
        dst = 0;
    }
    else
    {
        // information not valid
        dst = 0;
    }

    seconds += dst;

    if ((time_t)seconds != seconds)
        return (time_t)-1;
    return (time_t)seconds;
}
