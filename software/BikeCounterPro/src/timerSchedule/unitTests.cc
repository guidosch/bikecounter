#include <gtest/gtest.h>
#include "timerSchedule.hpp"

using namespace date;
using namespace std::chrono;
using namespace std::chrono_literals;

class TimerScheduleTest : public ::testing::Test
{
protected:
    // void SetUp() override {}
    // void TearDown() override {}
};

TEST_F(TimerScheduleTest, BasicIntervalTests)
{
    TimerSchedule ts1;

    // first interval
    // time_point 2022/01/03 00:30:00
    time_point<system_clock, seconds> t1 = sys_days(year_month_day(year{2022}, month{1}, day{3})) + hours{0} + minutes{30} + seconds{0};
    // solution: 2022/01/03 06:30:00
    time_point<system_clock, seconds> t1sol = t1 + 6h;
    ASSERT_EQ(ts1.getNextIntervalTime(t1).time_since_epoch().count(), t1sol.time_since_epoch().count());

    // second interval
    // time_point 2022/03/25 10:00:00
    time_point<system_clock, seconds> t2 = sys_days(year_month_day(year{2022}, month{4}, day{25})) + hours{10} + minutes{0} + seconds{0};
    // solution: 2022/03/25 12:00:00
    time_point<system_clock, seconds> t2sol = t2 + 2h;
    ASSERT_EQ(ts1.getNextIntervalTime(t2).time_since_epoch().count(), t2sol.time_since_epoch().count());

    // last interval
    // time_point 2022/01/03 17:13:46
    time_point<system_clock, seconds> t3 = sys_days(year_month_day(year{2022}, month{1}, day{3})) + hours{17} + minutes{12} + seconds{46};
    // solution: 2022/01/03 23:13:46
    time_point<system_clock, seconds> t3sol = t3 + 6h;
    ASSERT_EQ(ts1.getNextIntervalTime(t3).time_since_epoch().count(), t3sol.time_since_epoch().count());
}

TEST_F(TimerScheduleTest, LastCallInIntervalTests)
{
    TimerSchedule ts1;

    // first interval
    // time_point 2022/03/25 04:45:13
    time_point<system_clock, seconds> t1 = sys_days(year_month_day(year{2022}, month{3}, day{25})) + hours{4} + minutes{45} + seconds{13};
    // solution: 2022/03/25 05:01:00
    time_point<system_clock, seconds> t1sol = sys_days(year_month_day(year{2022}, month{3}, day{25})) + hours{5} + minutes{1} + seconds{0};
    ASSERT_EQ(ts1.getNextIntervalTime(t1).time_since_epoch().count(), t1sol.time_since_epoch().count());

    // second interval
    // time_point 2022/03/25 18:35:00
    time_point<system_clock, seconds> t2 = sys_days(year_month_day(year{2022}, month{3}, day{25})) + hours{18} + minutes{35} + seconds{0};
    // solution: 2022/03/25 19:01:00
    time_point<system_clock, seconds> t2sol = sys_days(year_month_day(year{2022}, month{3}, day{25})) + hours{19} + minutes{1} + seconds{0};
    ASSERT_EQ(ts1.getNextIntervalTime(t2).time_since_epoch().count(), t2sol.time_since_epoch().count());
}

TEST_F(TimerScheduleTest, LastCallOfTheDayTests)
{
    TimerSchedule ts1;

    // last call of the day
    // time_point 2022/03/25 21:47:39
    time_point<system_clock, seconds> t1 = sys_days(year_month_day(year{2022}, month{3}, day{25})) + hours{21} + minutes{47} + seconds{39};
    // solution: 2022/03/25 23:50:00
    time_point<system_clock, seconds> t1sol = sys_days(year_month_day(year{2022}, month{3}, day{25})) + hours{23} + minutes{50} + seconds{0};
    ASSERT_EQ(ts1.getNextIntervalTime(t1).time_since_epoch().count(), t1sol.time_since_epoch().count());
    // next call (on next day)
    // solution: 2022/03/26 00:01:00
    time_point<system_clock, seconds> t2sol = sys_days(year_month_day(year{2022}, month{3}, day{26})) + hours{0} + minutes{1} + seconds{0};
    ASSERT_EQ(ts1.getNextIntervalTime(t1sol).time_since_epoch().count(), t2sol.time_since_epoch().count());
}

TEST_F(TimerScheduleTest, LastCallOfTheMonthTests)
{
    TimerSchedule ts1;

    // last call of the day
    // time_point 2022/03/31 21:47:39
    time_point<system_clock, seconds> t1 = sys_days(year_month_day(year{2022}, month{3}, day{31})) + hours{21} + minutes{47} + seconds{39};
    // solution: 2022/03/31 23:50:00
    time_point<system_clock, seconds> t1sol = sys_days(year_month_day(year{2022}, month{3}, day{31})) + hours{23} + minutes{50} + seconds{0};
    ASSERT_EQ(ts1.getNextIntervalTime(t1).time_since_epoch().count(), t1sol.time_since_epoch().count());
    // next call (on next day)
    // solution: 2022/04/01 00:01:00
    time_point<system_clock, seconds> t2sol = sys_days(year_month_day(year{2022}, month{4}, day{1})) + hours{0} + minutes{1} + seconds{0};
    ASSERT_EQ(ts1.getNextIntervalTime(t1sol).time_since_epoch().count(), t2sol.time_since_epoch().count());
}

TEST_F(TimerScheduleTest, CurrentIntervalTests)
{
    TimerSchedule ts1;

    // first interval
    // time_point 2022/03/25 04:45:13
    time_point<system_clock, seconds> t1 = sys_days(year_month_day(year{2022}, month{3}, day{25})) + hours{4} + minutes{45} + seconds{13};
    // 6h
    ASSERT_EQ(ts1.getCurrentIntervalSeconds(t1), 6 * 60 * 60);
    ASSERT_EQ(ts1.getCurrentIntervalMinutes(t1), 6 * 60);

    // second interval
    // time_point 2022/03/25 18:35:00
    time_point<system_clock, seconds> t2 = sys_days(year_month_day(year{2022}, month{3}, day{25})) + hours{18} + minutes{35} + seconds{0};
    // 2h
    ASSERT_EQ(ts1.getCurrentIntervalSeconds(t2), 2 * 60 * 60);
    ASSERT_EQ(ts1.getCurrentIntervalMinutes(t2), 2 * 60);
}