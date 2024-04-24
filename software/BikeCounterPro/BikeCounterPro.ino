#include "config.h"
#include "src/bikeCounter/bikeCounter.hpp"

BikeCounter *bc = BikeCounter::getInstance();

void setup()
{
  bc->setCounterInterruptPin(0);
  bc->setSwitchPowerPin(10);
  bc->setDebugSwitchPin(7);
  bc->setConfigSwitchPin(8);
  bc->setBatteryVoltagePin(A0);
  bc->setPirPowerPin(3);
  bc->setSyncTimeInterval(120ul);  // 2*60 s
  bc->setMaxBlinks(50);
  bc->setMaxCount(1000);

  bc->loop();
}

// The main loop gets executed after the device wakes up
// (caused by the timer or the motion detection interrupt)
void loop()
{
  bc->loop();
  delay(50);
}