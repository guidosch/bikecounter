#include <gtest/gtest.h>
#include "timerSchedule.hpp"

class TimerScheduleTest : public ::testing::Test
{
protected:
    TimerSchedule ts1;

    // void SetUp() override {}
    // void TearDown() override {}
};

TEST_F(TimerScheduleTest, EnvTests)
{
    // DateTime t1 = DateTime(2022, 1, 3, 0, 30, 0);
    tm tm1 = {0};
    tm1.tm_year = 122; // 2022 - 1900
    tm1.tm_mon = 0;
    tm1.tm_mday = 3;
    tm1.tm_hour = 0;
    tm1.tm_min = 30;
    tm1.tm_sec = 0;
    tm1.tm_isdst = 0;
    ASSERT_EQ(mktimeutc(&tm1), (time_t)1641169800ul);

    // DateTime t2 = DateTime(2022, 8, 25, 10, 0, 0);
    tm tm2 = {0};
    tm2.tm_year = 122; // 2022 - 1900
    tm2.tm_mon = 7;
    tm2.tm_mday = 25;
    tm2.tm_hour = 10;
    tm2.tm_min = 0;
    tm2.tm_sec = 0;
    tm2.tm_isdst = 1;
    ASSERT_EQ(mktimeutc(&tm2), (time_t)1661421600ul);
}

TEST_F(TimerScheduleTest, BasicIntervalTests)
{
    // first interval
    // DateTime t1 = DateTime(2022, 1, 3, 0, 30, 0);
    tm tm1 = {0};
    tm1.tm_year = 122; // 2022 - 1900
    tm1.tm_mon = 0;
    tm1.tm_mday = 3;
    tm1.tm_hour = 0;
    tm1.tm_min = 30;
    tm1.tm_sec = 0;
    tm1.tm_isdst = 0;
    time_t t1 = mktimeutc(&tm1);

    // DateTime(2022, 1, 3, 6, 30, 0)
    tm tmR11 = tm1;
    tmR11.tm_hour = 6;
    time_t tR11 = mktimeutc(&tmR11);
    ASSERT_EQ(ts1.getNextIntervalTime(t1), tR11);

    // second interval
    // DateTime t2 = DateTime(2022, 3, 25, 10, 0, 0);
    tm tm2 = {0};
    tm2.tm_year = 122; // 2022 - 1900
    tm2.tm_mon = 2;
    tm2.tm_mday = 25;
    tm2.tm_hour = 10;
    tm2.tm_min = 0;
    tm2.tm_sec = 0;
    tm2.tm_isdst = 0;
    time_t t2 = mktimeutc(&tm2);

    // DateTime(2022, 3, 25, 12, 0, 0)
    tm tmR21 = tm2;
    tmR21.tm_hour = 12;
    time_t tR21 = mktimeutc(&tmR21);
    ASSERT_EQ(ts1.getNextIntervalTime(t2), tR21);

    // last interval
    // DateTime t3 = DateTime(2022, 1, 3, 17, 13, 46);
    tm tm3 = {0};
    tm3.tm_year = 122; // 2022 - 1900
    tm3.tm_mon = 0;
    tm3.tm_mday = 3;
    tm3.tm_hour = 17;
    tm3.tm_min = 13;
    tm3.tm_sec = 46;
    tm3.tm_isdst = 0;
    time_t t3 = mktimeutc(&tm3);

    // DateTime(2022, 1, 3, 23, 13, 46)
    tm tmR31 = tm3;
    tmR31.tm_hour = 23;
    time_t tR31 = mktimeutc(&tmR31);
    ASSERT_EQ(ts1.getNextIntervalTime(t3), tR31);
}

TEST_F(TimerScheduleTest, LastCallInIntervalTests)
{
    // first interval
    // DateTime t1 = DateTime(2022, 3, 25, 4, 45, 13);
    tm tm1 = {0};
    tm1.tm_year = 122; // 2022 - 1900
    tm1.tm_mon = 2;
    tm1.tm_mday = 25;
    tm1.tm_hour = 4;
    tm1.tm_min = 45;
    tm1.tm_sec = 13;
    tm1.tm_isdst = 0;
    time_t t1 = mktimeutc(&tm1);

    // DateTime(2022, 3, 25, 5, 1, 0)
    tm tmR11 = tm1;
    tmR11.tm_hour = 5;
    tmR11.tm_min = 1;
    tmR11.tm_sec = 0;
    time_t tR11 = mktimeutc(&tmR11);
    ASSERT_EQ(ts1.getNextIntervalTime(t1), tR11);

    // second interval
    // DateTime t2 = DateTime(2022, 3, 25, 18, 35, 0);
    tm tm2 = {0};
    tm2.tm_year = 122; // 2022 - 1900
    tm2.tm_mon = 2;
    tm2.tm_mday = 25;
    tm2.tm_hour = 18;
    tm2.tm_min = 35;
    tm2.tm_sec = 0;
    tm2.tm_isdst = 0;
    time_t t2 = mktimeutc(&tm2);

    // DateTime(2022, 3, 25, 19, 1, 0)
    tm tmR21 = tm2;
    tmR21.tm_hour = 19;
    tmR21.tm_min = 1;
    time_t tR21 = mktimeutc(&tmR21);
    ASSERT_EQ(ts1.getNextIntervalTime(t2), tR21);
}

TEST_F(TimerScheduleTest, LastCallOfTheDayTests)
{
    // last call of the day
    // DateTime t1 = DateTime(2022, 3, 25, 21, 47, 39);
    tm tm1 = {0};
    tm1.tm_year = 122; // 2022 - 1900
    tm1.tm_mon = 2;
    tm1.tm_mday = 25;
    tm1.tm_hour = 21;
    tm1.tm_min = 47;
    tm1.tm_sec = 39;
    tm1.tm_isdst = 0;
    time_t t1 = mktimeutc(&tm1);

    // DateTime(2022, 3, 25, 23, 49, 59)
    tm tmR11 = tm1;
    tmR11.tm_hour = 23;
    tmR11.tm_min = 50;
    tmR11.tm_sec = 0;
    time_t tR11 = mktimeutc(&tmR11);
    ASSERT_EQ(ts1.getNextIntervalTime(t1), tR11);
}

TEST_F(TimerScheduleTest, CurrentIntervalTests)
{
    // first interval
    // DateTime t1 = DateTime(2022, 3, 25, 4, 45, 13);
    tm tm1 = {0};
    tm1.tm_year = 122; // 2022 - 1900
    tm1.tm_mon = 2;
    tm1.tm_mday = 25;
    tm1.tm_hour = 4;
    tm1.tm_min = 45;
    tm1.tm_sec = 13;
    tm1.tm_isdst = 0;
    time_t t1 = mktimeutc(&tm1);

    // TimeSpan(0, 6, 0, 0)
    ASSERT_EQ(ts1.getCurrentIntervalSeconds(t1), 6 * 60 * 60);
    ASSERT_EQ(ts1.getCurrentIntervalMinutes(t1), 6 * 60);

    // second interval
    // DateTime t2 = DateTime(2022, 3, 25, 18, 35, 0);
    tm tm2 = {0};
    tm2.tm_year = 122; // 2022 - 1900
    tm2.tm_mon = 2;
    tm2.tm_mday = 25;
    tm2.tm_hour = 18;
    tm2.tm_min = 35;
    tm2.tm_sec = 0;
    tm2.tm_isdst = 0;
    time_t t2 = mktimeutc(&tm2);

    // TimeSpan(0, 2, 0, 0)
    ASSERT_EQ(ts1.getCurrentIntervalSeconds(t2), 2 * 60 * 60);
    ASSERT_EQ(ts1.getCurrentIntervalMinutes(t2), 2 * 60);
}