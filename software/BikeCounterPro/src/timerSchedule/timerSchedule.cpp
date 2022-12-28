#include "timerSchedule.hpp"

using namespace date;
using namespace std::chrono;

time_point<system_clock, seconds> TimerSchedule::getNextIntervalTime(time_point<system_clock, seconds> currentDateTime)
{
    year_month_day cdt_ymd{floor<days>(currentDateTime)};
    hh_mm_ss<seconds> cdt_hms = make_time(currentDateTime.time_since_epoch() - floor<days>(currentDateTime).time_since_epoch());
    int cId = getIntervalId(cdt_hms.hours().count(), unsigned{cdt_ymd.month()});

    time_point<system_clock, seconds> new_dt = currentDateTime + seconds{timeSpanIntervals[cId]};

    year_month_day new_dt_ymd{floor<days>(new_dt)};
    hh_mm_ss<seconds> new_dt_hms = make_time(new_dt.time_since_epoch() - floor<days>(new_dt).time_since_epoch());

    // Check if next call is inside same day
    if ((floor<days>(new_dt) != floor<days>(currentDateTime)) && !isLastCall)
    {
        new_dt = sys_days{cdt_ymd} + hours{23} + minutes{50};
        isLastCall = true;
    }
    else
    {
        if (isLastCall)
        {
            new_dt = sys_days{cdt_ymd} + days{1} + minutes{1};
        }
        else
        {
            // Check if next call is inside same interval
            // if not change the next call to the start of the new interval to sync the timing
            int nId = getIntervalId(new_dt_hms.hours().count(), unsigned{new_dt_ymd.month()});
            if (nId != cId)
            {
                new_dt = floor<days>(currentDateTime) + hours{IntervalStartUTC[nId][unsigned{new_dt_ymd.month()} - 1]} + minutes{1};
            }
        }
        isLastCall = false;
    }
    return new_dt;
};

uint32_t TimerSchedule::getCurrentIntervalSeconds(time_point<system_clock, seconds> cDT)
{
    year_month_day cdt_ymd{floor<days>(cDT)};
    hh_mm_ss<seconds> cdt_hms = make_time(cDT.time_since_epoch() - floor<days>(cDT).time_since_epoch());
    int cId = getIntervalId(cdt_hms.hours().count(), unsigned{cdt_ymd.month()});
    return timeSpanIntervals[cId];
};

uint32_t TimerSchedule::getCurrentIntervalMinutes(time_point<system_clock, seconds> cDT)
{
    return (uint32_t)(TimerSchedule::getCurrentIntervalSeconds(cDT) / 60);
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
