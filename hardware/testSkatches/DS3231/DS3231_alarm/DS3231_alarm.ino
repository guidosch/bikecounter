/* Example implementation of an alarm using DS3231

   VCC and GND of RTC should be connected to some power source
   SDA, SCL of RTC should be connected to SDA, SCL of arduino
   SQW should be connected to CLOCK_INTERRUPT_PIN
   CLOCK_INTERRUPT_PIN needs to work with interrupts
*/

#include <RTClib.h>
// #include <Wire.h>

RTC_DS3231 rtc;

// the pin that is connected to SQW
#define CLOCK_INTERRUPT_PIN 1

void setup() {
  Serial.begin(9600);

  // initializing the rtc
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC!");
    Serial.flush();
    abort();
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  //we don't need the 32K Pin, so disable it
  rtc.disable32K();

  // Making it so, that the alarm will trigger an interrupt
  pinMode(CLOCK_INTERRUPT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(CLOCK_INTERRUPT_PIN), onAlarm, FALLING);

  // set alarm 1, 2 flag to false (so alarm 1, 2 didn't happen so far)
  // if not done, this easily leads to problems, as both register aren't reset on reboot/recompile
  rtc.clearAlarm(1);
  rtc.clearAlarm(2);

  // stop oscillating signals at SQW Pin
  // otherwise setAlarm1 will fail
  rtc.writeSqwPinMode(DS3231_OFF);

  // turn off alarm 2 (in case it isn't off already)
  // again, this isn't done at reboot, so a previously set alarm could easily go overlooked
  rtc.disableAlarm(2);

  // schedule an alarm 10 seconds in the future
  resetAlarm1();
}

void loop() {
  // print current time
  char date[10] = "hh:mm:ss";
  rtc.now().toString(date);
  Serial.print(date);
  // the value at SQW-Pin (because of pullup 1 means no alarm)
  Serial.print(" SQW: ");
  Serial.print(digitalRead(CLOCK_INTERRUPT_PIN));
  // whether a alarm happened happened
  Serial.print(" Alarm1: ");
  Serial.print(rtc.alarmFired(1));
  Serial.print(" Alarm2: ");
  Serial.println(rtc.alarmFired(2));
  // control register values (see https://datasheets.maximintegrated.com/en/ds/DS3231.pdf page 13)
  // Serial.print(" Control: 0b");
  // Serial.println(read_i2c_register(DS3231_ADDRESS, DS3231_CONTROL), BIN);

  // resetting SQW and alarm 1 flag
  // using setAlarm1, the next alarm could now be configurated
  if (rtc.alarmFired(1)) {
    resetAlarm1();
  }

  delay(2000);
}

void onAlarm() {
  Serial.println("Alarm occured!");
}

void resetAlarm1()
{
  rtc.clearAlarm(1);
  if (!rtc.setAlarm1(
        rtc.now() + TimeSpan(10),
        DS3231_A1_Second // this mode triggers the alarm when the seconds match.
      )) {
    Serial.println("Error, alarm wasn't set!");
  }
}

/*static uint8_t read_i2c_register(uint8_t addr, uint8_t reg) {
    Wire.beginTransmission(addr);
    Wire.write((byte)reg);
    Wire.endTransmission();

    Wire.requestFrom(addr, (byte)1);
    return Wire.read();
  }*/
