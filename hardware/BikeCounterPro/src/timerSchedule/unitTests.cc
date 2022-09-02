#include <gtest/gtest.h>
#include "timerSchedule.hpp"

class TimerScheduleTest : public ::testing::Test
{
protected:
    TimerSchedule ts1;

    // void SetUp() override {}
    // void TearDown() override {}
};

TEST_F(TimerScheduleTest, BasicIntervalTests)
{
    // first interval
    // DateTime t1 = DateTime(2022, 1, 3, 0, 30, 0);
    tm tm1;
    tm1.tm_year = 122; // 2022 - 1900
    tm1.tm_mon = 0;
    tm1.tm_mday = 3;
    tm1.tm_hour = 0;
    tm1.tm_min = 30;
    tm1.tm_sec = 0;
    time_t t1 = mktime(&tm1) - _timezone;

    // DateTime(2022, 1, 3, 6, 30, 0)
    tm tmR11 = tm1;
    tmR11.tm_hour = 6;
    time_t tR11 = mktime(&tmR11) - _timezone;
    EXPECT_EQ(ts1.getNextIntervalTime(t1), tR11);

    // DateTime(2022, 1, 3, 2, 30, 0)
    tm tmR12 = tm1;
    tmR12.tm_hour = 2;
    time_t tR12 = mktime(&tmR12) - _timezone;
    EXPECT_NE(ts1.getNextIntervalTime(t1), tR12);

    // DateTime(2022, 1, 3, 7, 30, 0)
    tm tmR13 = tm1;
    tmR13.tm_hour = 7;
    time_t tR13 = mktime(&tmR13) - _timezone;
    EXPECT_NE(ts1.getNextIntervalTime(t1), tR13);

    // second interval
    // DateTime t2 = DateTime(2022, 3, 25, 10, 0, 0);
    tm tm2;
    tm2.tm_year = 122; // 2022 - 1900
    tm2.tm_mon = 2;
    tm2.tm_mday = 25;
    tm2.tm_hour = 10;
    tm2.tm_min = 0;
    tm2.tm_sec = 0;
    time_t t2 = mktime(&tm2) - _timezone;

    // DateTime(2022, 3, 25, 12, 0, 0)
    tm tmR21 = tm2;
    tmR21.tm_hour = 12;
    time_t tR21 = mktime(&tmR21) - _timezone;
    EXPECT_EQ(ts1.getNextIntervalTime(t2), tR21);

    // DateTime(2022, 3, 25, 16, 0, 0)
    tm tmR22 = tm2;
    tmR22.tm_hour = 16;
    time_t tR22 = mktime(&tmR22) - _timezone;
    EXPECT_NE(ts1.getNextIntervalTime(t2), tR22);

    // DateTime(2023, 4, 26, 12, 0, 0)
    tm tmR23 = tm2;
    tmR23.tm_year = 123;
    tmR23.tm_mon = 3;
    tmR23.tm_mday = 26;
    tmR23.tm_hour = 12;
    time_t tR23 = mktime(&tmR23) - _timezone;
    EXPECT_NE(ts1.getNextIntervalTime(t2), tR23);

    // last interval
    // DateTime t3 = DateTime(2022, 1, 3, 17, 13, 46);
    tm tm3;
    tm3.tm_year = 122; // 2022 - 1900
    tm3.tm_mon = 0;
    tm3.tm_mday = 3;
    tm3.tm_hour = 17;
    tm3.tm_min = 13;
    tm3.tm_sec = 46;
    time_t t3 = mktime(&tm3) - _timezone;

    // DateTime(2022, 1, 3, 23, 13, 46)
    tm tmR31 = tm3;
    tmR31.tm_hour = 23;
    time_t tR31 = mktime(&tmR31) - _timezone;
    EXPECT_EQ(ts1.getNextIntervalTime(t3), tR31);

    // DateTime(2022, 1, 3, 19, 13, 46)
    tm tmR32 = tm3;
    tmR32.tm_hour = 19;
    time_t tR32 = mktime(&tmR32) - _timezone;
    EXPECT_NE(ts1.getNextIntervalTime(t3), tR32);

    // DateTime(2022, 1, 3, 23, 13, 20)
    tm tmR33 = tm3;
    tmR33.tm_hour = 23;
    tmR33.tm_sec = 20;
    time_t tR33 = mktime(&tmR33) - _timezone;
    EXPECT_NE(ts1.getNextIntervalTime(t3), tR33);
}

TEST_F(TimerScheduleTest, LastCallInIntervalTests)
{
    // first interval
    // DateTime t1 = DateTime(2022, 3, 25, 4, 45, 13);
    tm tm1;
    tm1.tm_year = 122; // 2022 - 1900
    tm1.tm_mon = 2;
    tm1.tm_mday = 25;
    tm1.tm_hour = 4;
    tm1.tm_min = 45;
    tm1.tm_sec = 13;
    time_t t1 = mktime(&tm1) - _timezone;

    // DateTime(2022, 3, 25, 5, 1, 0)
    tm tmR11 = tm1;
    tmR11.tm_hour = 5;
    tmR11.tm_min = 1;
    tmR11.tm_sec = 0;
    time_t tR11 = mktime(&tmR11) - _timezone;
    EXPECT_EQ(ts1.getNextIntervalTime(t1), tR11);

    // DateTime(2022, 3, 25, 10, 45, 13)
    tm tmR12 = tm1;
    tmR12.tm_hour = 10;
    time_t tR12 = mktime(&tmR12) - _timezone;
    EXPECT_NE(ts1.getNextIntervalTime(t1), tR12);

    // DateTime(2022, 3, 25, 6, 45, 13)
    tm tmR13 = tm1;
    tmR13.tm_hour = 6;
    time_t tR13 = mktime(&tmR13) - _timezone;
    EXPECT_NE(ts1.getNextIntervalTime(t1), tR13);

    // second interval
    // DateTime t2 = DateTime(2022, 3, 25, 18, 35, 0);
    tm tm2;
    tm2.tm_year = 122; // 2022 - 1900
    tm2.tm_mon = 2;
    tm2.tm_mday = 25;
    tm2.tm_hour = 18;
    tm2.tm_min = 35;
    tm2.tm_sec = 0;
    time_t t2 = mktime(&tm2) - _timezone;

    // DateTime(2022, 3, 25, 19, 1, 0)
    tm tmR21 = tm2;
    tmR21.tm_hour = 19;
    tmR21.tm_min = 1;
    time_t tR21 = mktime(&tmR21) - _timezone;
    EXPECT_EQ(ts1.getNextIntervalTime(t2), tR21);

    // DateTime(2022, 3, 25, 20, 35, 0)
    tm tmR22 = tm2;
    tmR22.tm_hour = 20;
    time_t tR22 = mktime(&tmR22) - _timezone;
    EXPECT_NE(ts1.getNextIntervalTime(t2), tR22);

    // DateTime(2022, 3, 26, 0, 35, 0)
    tm tmR23 = tm2;
    tmR23.tm_mday = 26;
    tmR23.tm_hour = 0;
    time_t tR23 = mktime(&tmR23) - _timezone;
    EXPECT_NE(ts1.getNextIntervalTime(t2), tR23);
}

TEST_F(TimerScheduleTest, LastCallOfTheDayTests)
{
    // last call of the day
    // DateTime t1 = DateTime(2022, 3, 25, 21, 47, 39);
    tm tm1;
    tm1.tm_year = 122; // 2022 - 1900
    tm1.tm_mon = 2;
    tm1.tm_mday = 25;
    tm1.tm_hour = 21;
    tm1.tm_min = 47;
    tm1.tm_sec = 39;
    time_t t1 = mktime(&tm1) - _timezone;

    // DateTime(2022, 3, 25, 23, 49, 59)
    tm tmR11 = tm1;
    tmR11.tm_hour = 23;
    tmR11.tm_min = 50;
    tmR11.tm_sec = 0;
    time_t tR11 = mktime(&tmR11) - _timezone;
    EXPECT_EQ(ts1.getNextIntervalTime(t1), tR11);

    // DateTime(2022, 3, 26, 3, 47, 39)
    tm tmR12 = tm1;
    tmR12.tm_mday = 26;
    tmR12.tm_hour = 3;
    time_t tR12 = mktime(&tmR12) - _timezone;
    EXPECT_NE(ts1.getNextIntervalTime(t1), tR12);

    // DateTime(2022, 3, 25, 23, 47, 39)
    tm tmR13 = tm1;
    tmR13.tm_hour = 23;
    time_t tR13 = mktime(&tmR13) - _timezone;
    EXPECT_NE(ts1.getNextIntervalTime(t1), tR13);
}

TEST_F(TimerScheduleTest, CurrentIntervalTests)
{
    // first interval
    // DateTime t1 = DateTime(2022, 3, 25, 4, 45, 13);
    tm tm1;
    tm1.tm_year = 122; // 2022 - 1900
    tm1.tm_mon = 2;
    tm1.tm_mday = 25;
    tm1.tm_hour = 4;
    tm1.tm_min = 45;
    tm1.tm_sec = 13;
    time_t t1 = mktime(&tm1) - _timezone;

    // TimeSpan(0, 6, 0, 0)
    EXPECT_EQ(ts1.getCurrentIntervalSeconds(t1), 6 * 60 * 60);
    EXPECT_EQ(ts1.getCurrentIntervalMinutes(t1), 6 * 60);
    // TimeSpan(0, 2, 0, 0)
    EXPECT_NE(ts1.getCurrentIntervalSeconds(t1), 2 * 60 * 60);
    EXPECT_NE(ts1.getCurrentIntervalMinutes(t1), 2 * 60);

    // second interval
    // DateTime t2 = DateTime(2022, 3, 25, 18, 35, 0);
    tm tm2;
    tm2.tm_year = 122; // 2022 - 1900
    tm2.tm_mon = 2;
    tm2.tm_mday = 25;
    tm2.tm_hour = 18;
    tm2.tm_min = 35;
    tm2.tm_sec = 0;
    time_t t2 = mktime(&tm2) - _timezone;

    // TimeSpan(0, 2, 0, 0)
    EXPECT_EQ(ts1.getCurrentIntervalSeconds(t2), 2 * 60 * 60);
    EXPECT_EQ(ts1.getCurrentIntervalMinutes(t2), 2 * 60);
    // TimeSpan(0, 6, 0, 0)
    EXPECT_NE(ts1.getCurrentIntervalSeconds(t2), 6 * 60 * 60);
    EXPECT_NE(ts1.getCurrentIntervalMinutes(t2), 6 * 60);
}