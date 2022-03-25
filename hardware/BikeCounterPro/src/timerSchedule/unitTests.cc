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
    DateTime t1 = DateTime(2022, 1, 3, 0, 30, 0);
    EXPECT_TRUE(ts1.getNextIntervalTime(t1) == DateTime(2022, 1, 3, 6, 30, 0));
    EXPECT_FALSE(ts1.getNextIntervalTime(t1) == DateTime(2022, 1, 3, 2, 30, 0));
    EXPECT_FALSE(ts1.getNextIntervalTime(t1) == DateTime(2022, 1, 3, 7, 30, 0));

    // second interval
    DateTime t2 = DateTime(2022, 3, 25, 10, 0, 0);
    EXPECT_TRUE(ts1.getNextIntervalTime(t2) == DateTime(2022, 3, 25, 12, 0, 0));
    EXPECT_FALSE(ts1.getNextIntervalTime(t2) == DateTime(2022, 3, 25, 16, 0, 0));
    EXPECT_FALSE(ts1.getNextIntervalTime(t2) == DateTime(2023, 4, 26, 12, 0, 0));

    // last interval
    DateTime t3 = DateTime(2022, 1, 3, 17, 13, 46);
    EXPECT_TRUE(ts1.getNextIntervalTime(t3) == DateTime(2022, 1, 3, 23, 13, 46));
    EXPECT_FALSE(ts1.getNextIntervalTime(t3) == DateTime(2022, 1, 3, 19, 13, 46));
    EXPECT_FALSE(ts1.getNextIntervalTime(t3) == DateTime(2022, 1, 3, 23, 13, 20));
}

TEST_F(TimerScheduleTest, LastCallInIntervalTests)
{
    // first interval
    DateTime t1 = DateTime(2022, 3, 25, 4, 45, 13);
    EXPECT_TRUE(ts1.getNextIntervalTime(t1) == DateTime(2022, 3, 25, 5, 1, 0));
    EXPECT_FALSE(ts1.getNextIntervalTime(t1) == DateTime(2022, 3, 25, 10, 45, 13));
    EXPECT_FALSE(ts1.getNextIntervalTime(t1) == DateTime(2022, 3, 25, 6, 45, 13));

    // second interval
    DateTime t2 = DateTime(2022, 3, 25, 18, 35, 0);
    EXPECT_TRUE(ts1.getNextIntervalTime(t2) == DateTime(2022, 3, 25, 19, 1, 0));
    EXPECT_FALSE(ts1.getNextIntervalTime(t2) == DateTime(2022, 3, 25, 20, 35, 0));
    EXPECT_FALSE(ts1.getNextIntervalTime(t2) == DateTime(2022, 3, 26, 0, 35, 0));
}

TEST_F(TimerScheduleTest, LastCallOfTheDayTests)
{
    // last call of the day
    DateTime t1 = DateTime(2022, 3, 25, 21, 47, 39);
    EXPECT_TRUE(ts1.getNextIntervalTime(t1) == DateTime(2022, 3, 25, 23, 49, 59));
    EXPECT_FALSE(ts1.getNextIntervalTime(t1) == DateTime(2022, 3, 26, 3, 47, 39));
    EXPECT_FALSE(ts1.getNextIntervalTime(t1) == DateTime(2022, 3, 25, 23, 47, 39));
}