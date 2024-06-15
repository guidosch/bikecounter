#include "config.h"
#include "src/bikeCounter/bikeCounter.hpp"
#include "src/hal/hal_arduino.hpp"

BikeCounter *bc = BikeCounter::getInstance();
HAL *hal = HAL_Arduino::getInstance();

void setup()
{
  bc->injectHal(hal);
  bc->setCounterInterruptPin(0);
  bc->setSwitchPowerPin(10);
  bc->setDebugSwitchPin(7);
  bc->setConfigSwitchPin(8);
  bc->setBatteryVoltagePin(A0);
  bc->setPirPowerPin(3);
  bc->setSyncTimeInterval(120ul); // 2*60 s
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