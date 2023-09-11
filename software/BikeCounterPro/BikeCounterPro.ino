#include "config.h"
#include "src/bikeCounter/bikeCounter.hpp"

BikeCounter bc = BikeCounter();

void setup()
{
  bc.setCounterInterruptPin(1);
  bc.setDebugSwitchPin(8);
  bc.setConfigSwitchPin(9);
  bc.setBatteryVoltagePin(A0);
  bc.setPirPowerPin(2);
  bc.setDebugSleepTime(300000ul); // 5*60*1000 ms
  bc.setSyncTimeInterval(120ul);  // 2*60 s
  bc.setMaxBlinks(50);
  bc.setMaxCount(1000);
}

// The main loop gets executed after the device wakes up
// (caused by the timer or the motion detection interrupt)
void loop()
{
  // This statement simulates the sleep/deepSleep method in debug mode.
  // The reason for not using the sleep or deepSleep method is, that it disables
  // the usb connection which leeds to problems with the serial monitor.
  if (debugFlag)
  {
    // Check if a motion was detected or the sleep time expired
    // (Implemented in a nested if-statement to give the compiler the opportunity
    // to remove the whole outer statement depending on the constexpr debugFlag.)
    uint32_t sleepTime = ((deviceStatus != sync_call) && (lastDeviceStatus != sync_call)) ? debugSleepTime : (syncTimeInterval * 1000); // sync call interval
    if ((motionDetected) || ((millis() - lastMillis) >= sleepTime))
    {
      // Run the main loop once
      lastMillis = millis();
    }
    else
    {
      // Noting happened, continue with the next cycle
      return;
    }
  }
  else
  {
    // RTC bug prevention
    // If the device runs on battery the rtc seems to reinitialize it's register after the first sleep period.
    // To avoid this a sleep is triggered in the first loop and the rtc time will be reset after waking up.
    if (firstLoop)
    {
      firstLoop = 0;
      LowPower.deepSleep(2000);
    }
    if (firstWakeUp)
    {
      firstWakeUp = 0;
      rtc.setEpoch(defaultRTCEpoch);
    }
  }

  // get current time
  std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds> currentTime{std::chrono::seconds{rtc.getEpoch()}};
  date::hh_mm_ss<std::chrono::seconds> currentTime_hms = date::make_time(currentTime.time_since_epoch() - date::floor<date::days>(currentTime).time_since_epoch());
  int timerCalled = 0;

  logger.push(String("Device status = ") +
              String(deviceStatus) +
              String(" / Last device status = ") +
              String(lastDeviceStatus));
  logger.loop();

  // check if a motion was detected.
  if (motionDetected)
  {
    motionDetected = 0;

    // set hour of the day if this was the first call
    if (counter == 0)
    {
      hourOfDay = currentTime_hms.hours().count();
    }
    timeArray[counter] = (currentTime_hms.hours().count() - hourOfDay) * 60 + currentTime_hms.minutes().count();

    ++counter;
    ++totalCounter;

    blinkLED();

    logger.push(String("Motion detected (current count = ") +
                String(counter) +
                String(" / time: ") +
                String(static_cast<int>(currentTime_hms.hours().count())) +
                String(':') +
                String(static_cast<int>(currentTime_hms.minutes().count())) +
                String(':') +
                String(static_cast<int>(currentTime_hms.seconds().count())) +
                String(')'));
    logger.loop();
  }
  else
  {
    // if no motion was detected it means that the timer caused the wakeup.
    timerCalled = 1;
    lastDeviceStatus = deviceStatus;
  }

  // check if the data should be sent.
  int currentThreshold = dataHandler.getMaxCount(timeHandler.getCurrentIntervalMinutes(currentTime));
  if (((counter >= currentThreshold) || (timerCalled)) && (!isSending))
  {
    isSending = 1;
    // disable the pir sensor
    digitalWrite(pirPowerPin, LOW);

    if (timerCalled)
    {
      totalCounter = 0;
      logger.push("Timer called");
      logger.loop();
    }

    // check if the floating interrupt pin bug occurred
    // method 1: check if the totalCount exceeds the maxCount between the timer calls.
    // method 2: detect if the count goes up very quickly. (faster then the board is able to send)
    if ((totalCounter >= maxCount) || (counter > (currentThreshold + 5)))
    {
      ++pirError;
      if (pirError > 2)
      {
        logger.push("PIR-sensor error could not be fixed.");
        logger.loop();
        while (1)
        {
        }
      }
      logger.push("Floating interrupt pin detected.");
      logger.push("Resetting PIR-sensor");
      logger.loop();

      digitalWrite(pirPowerPin, LOW);
      delay(2000);
      totalCounter = 0;
    }

    blinkLED(2);
    sendData();

    delay(200);

    // update time
    currentTime = std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>(std::chrono::seconds{rtc.getEpoch()});

    // enable the pir sensor
    if (deviceStatus != sync_call)
    {
      digitalWrite(pirPowerPin, HIGH);
    }

    delay(200);

    isSending = 0;
  }

  delay(200);

  if (timerCalled)
  {
    // determine the sleep time if we're not in debug mode
    if ((deviceStatus != sync_call) && (lastDeviceStatus != sync_call))
    {
      nextAlarm = timeHandler.getNextIntervalTime(currentTime);
    }
    else
    {
      nextAlarm = currentTime + std::chrono::seconds{syncTimeInterval};
    }
    timerCalled = 0;
  }
  int64_t sdt = (nextAlarm.time_since_epoch().count() - currentTime.time_since_epoch().count());
  uint32_t sleepTime = sdt > 0 ? (uint32_t)sdt : syncTimeInterval;
  // sanity check
  if (sleepTime > (12ul * 60ul * 60ul))
  {
    sleepTime = 12ul * 60ul * 60ul;
  }
  if (!debugFlag)
  {
    LowPower.deepSleep(sleepTime * 1000ul);
  }
  else
  {
    logger.push(String("next non-debug wake up in: ") +
                String(sleepTime) +
                String(" seconds."));
    logger.loop();
  }
}

// ----------------------------------------------------
// --------- Method implementation section -----------
// ----------------------------------------------------

void onMotionDetected()
{
  if (!isSending)
  {
    motionDetected = 1;
  }
}

// void sendData()
//{
//   int err;
//   // data is transmitted as Ascii chars
//   modem.beginPacket();
//
//   dataHandler.setStatus(deviceStatus);
//   dataHandler.setHwVersion(hwVersion);
//   dataHandler.setSwVersion(swVersion);
//   dataHandler.setMotionCount(counter);
//   dataHandler.setBatteryVoltage(getBatteryVoltage());
//   dataHandler.setTemperature(am2320.readTemperature());
//   dataHandler.setHumidity(am2320.readHumidity());
//   dataHandler.setHourOfTheDay(hourOfDay);
//   dataHandler.setDeviceTime(rtc.getEpoch());
//   dataHandler.setTimeArray(timeArray);
//
//   modem.write(dataHandler.getPayload(), dataHandler.getPayloadLength());
//   err = modem.endPacket(false);
//   if (err > 0)
//   {
//     if (debugFlag)
//     {
//       Serial.print("Message sent correctly! (count = ");
//       Serial.print(counter);
//       Serial.print(" / temperature = ");
//       Serial.print(dataHandler.getTemperature());
//       Serial.print("°C / humidity = ");
//       Serial.print(dataHandler.getHumidity());
//       Serial.print("% / battery voltage = ");
//       Serial.print(dataHandler.getBatteryVoltage());
//       Serial.print(" V / DeviceEpoch = ");
//       Serial.print(dataHandler.getDeviceTime());
//       Serial.println(" )");
//       Serial.print("Payload = ");
//       for (int i = 0; i < dataHandler.getPayloadLength(); ++i)
//       {
//         Serial.print(dataHandler.getPayload()[i], HEX);
//         Serial.print(" ");
//       }
//       Serial.println();
//     }
//     counter = 0;
//     for (int i = 0; i < timeArraySize; ++i)
//     {
//       timeArray[i] = 0;
//     }
//   }
//   else
//   {
//     if (debugFlag)
//     {
//       Serial.println("Error sending message :(");
//     }
//     errorCounter++;
//     if (errorCounter > 1)
//     {
//       digitalWrite(LORA_RESET, LOW);
//       if (debugFlag)
//       {
//         Serial.println("Trying to reconnect");
//       }
//       doConnect();
//     }
//   }

//    // wait for all data transmission to finish (up- and downlink)
//    delay(10000);
//    // receive and decode downlink message
//    if (modem.available())
//    {
//      int rcv[64] = {0};
//      int i = 0;
//      while (modem.available())
//      {
//        rcv[i++] = modem.read();
//      }
//      if (debugFlag)
//      {
//        Serial.print("Received: ");
//        for (unsigned int j = 0; j < i; j++)
//        {
//          Serial.print(rcv[j] >> 4, HEX);
//          Serial.print(rcv[j] & 0xF, HEX);
//          Serial.print(" ");
//        }
//        Serial.println();
//      }
//
//      // decode time drift
//      int32_t timeDrift = 0;
//      timeDrift = rcv[3];
//      timeDrift = (timeDrift << 8) | rcv[2];
//      timeDrift = (timeDrift << 8) | rcv[1];
//      timeDrift = (timeDrift << 8) | rcv[0];
//      if (debugFlag)
//      {
//        Serial.print("Time drift = ");
//        Serial.println(timeDrift);
//      }
//
//      // check if the timeDrift should be applied
//      if ((abs(timeDrift) > (10 * 60)) && !skipTimeSync)
//      {
//        // apply time correction
//        correctRTCTime(timeDrift);
//        // skip the next downlinks to avoid multiple corrections with the same value (due to the network delay)
//        skipTimeSync = 1;
//        // change the status and power on the PIR sensor
//        if (deviceStatus == sync_call)
//        {
//          deviceStatus = no_error;
//        }
//      }
//    }
//    else
//    {
//      // set the flag to start listening to the downlink messages again.
//      skipTimeSync = 0;
//      if (debugFlag)
//      {
//        Serial.println("No downlink massage received.");
//      }
//    }
//  }

// apply the given correction to the rtc time
void correctRTCTime(int32_t delta)
{
  // get the current time
  uint32_t currentEpoch = rtc.getEpoch();
  // set the new time
  rtc.setEpoch(currentEpoch + delta);

  logger.push(String("RTC correction applied, current time: ") +
              String(rtc.getHours()) +
              String(':') +
              String(rtc.getMinutes()) +
              String(':') +
              String(rtc.getSeconds()));
  logger.push(String("RTC current date: ") +
              String(rtc.getDay()) +
              String('.') +
              String(rtc.getMonth()) +
              String('.') +
              String(rtc.getYear()));
  logger.push(String("RTC epoch: ") +
              String(rtc.getEpoch()));
  logger.loop();
  delay(500);
}