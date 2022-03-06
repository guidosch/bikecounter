#include <gtest/gtest.h>
#include "timerSchedule.hpp"

class TimerScheduleTest : public ::testing::Test
{
protected:
    // TimerSchedule t1;

    // void SetUp() override {}
    // void TearDown() override {}
};

TEST_F(TimerScheduleTest, BasicIntervalTests)
{
    DateTime at = DateTime(10);
    TimerSchedule t1 = TimerSchedule();
    t1.getNextIntervalTime(at);
    
    // Expect two strings not to be equal.
    EXPECT_STRNE("hello", "world");
    // Expect equality.
    EXPECT_EQ(7 * 6, 42);
}

TEST_F(TimerScheduleTest, LastCallInIntervalTests)
{
    // Expect two strings not to be equal.
    EXPECT_STRNE("hello", "world");
    // Expect equality.
    EXPECT_EQ(7 * 6, 42);
}

TEST_F(TimerScheduleTest, LastCallOfTheDayTests)
{
    // Expect two strings not to be equal.
    EXPECT_STRNE("hello", "world");
    // Expect equality.
    EXPECT_EQ(7 * 6, 42);
}