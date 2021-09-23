#include "ArduinoLowPower.h"
#include <RTClib.h>
#include <Wire.h>
#include "wiring_private.h"
TwoWire myWire(&sercom3, 0, 1);










// set debugFlag = 1 to activate serial debug messages and to disable deepSleep
const bool debugFlag = 1;

// RTC object
RTC_DS3231 rtc;
// Alarm interval (days, hours, minutes, seconds)
TimeSpan alarmInterval = TimeSpan(0, 0, 0, 20);

// Interrupt pins
const int timerInterruptPin = 4;

// Timer interrupt falg
bool timerCalled = 0;
// Lora data transmission flag
bool isSending = 0;
// Error counter for connection
int errorCounter = 0;

// Blink methode prototype
void blinkLED(int times = 1);

void setup() {
  // setup onboard LED
  pinMode(LED_BUILTIN, OUTPUT);

  if (debugFlag) {
    // open serial connection and wait
    Serial.begin(9600);
    while (!Serial);
  }




  myWire.begin(2);                // join i2c bus with address #2

  pinPeripheral(0, PIO_SERCOM);   //Assign SDA function to pin 0

  pinPeripheral(1, PIO_SERCOM);   //Assign SCL function to pin 1

  









  // setup rtc
  bool rtcConnection = rtc.begin(&myWire);

  if ((!rtcConnection) && debugFlag)
  {
    Serial.println("Couldn't find RTC!");
  }
  if (rtc.lostPower()) {
    if (debugFlag) {
      Serial.println("RTC lost power, let's set the time!");
    }
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
  //disabel the 32K pin
  rtc.disable32K();
  // setup timer interrupt
  pinMode(timerInterruptPin, INPUT_PULLUP);
  LowPower.attachInterruptWakeup(timerInterruptPin, onTimerInterrupt, FALLING);
  //attachInterrupt(digitalPinToInterrupt(1), onTimerInterrupt, FALLING);
  // clear alarms
  rtc.clearAlarm(1);
  rtc.clearAlarm(2);
  // stop oscillating signal at SQW Pin
  rtc.writeSqwPinMode(DS3231_OFF);
  // turn off alarm 2
  rtc.disableAlarm(2);
  // set next alarm
  setAlarm(alarmInterval);

  

  if (debugFlag)
  {
    Serial.println("Setup finished");
  }

  delay(200);

}

void loop() {

  if (timerCalled) {
    setAlarm(alarmInterval);
    timerCalled = 0;
  }


  if (!debugFlag) {
    delay(200);
    LowPower.sleep();
  }
}

void onTimerInterrupt() {
  if (!isSending) {
    timerCalled = 1;
    if (debugFlag) {
      Serial.println("Timer called");
    }
  }
}





// blinks the on board led
void blinkLED(int times) {
  for (int i = 0; i <= times; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);
    digitalWrite(LED_BUILTIN, LOW);
  }
}


void setAlarm(TimeSpan dt)
{
  rtc.clearAlarm(1);
  
  DateTime nextAlarm = rtc.now() + dt;
  if (dt.hours() > 0)
  {
    // alarm when the hours, the minutes and the seconds match.
    rtc.setAlarm1(nextAlarm, DS3231_A1_Hour);
  }
  else if (dt.minutes() > 0)
  {
    // alarm when the minutes and the seconds match.
    rtc.setAlarm1(nextAlarm, DS3231_A1_Minute);
  }
  else
  {
    // alarm when the seconds match.
    rtc.setAlarm1(nextAlarm, DS3231_A1_Second);
  }

}




// Attach the interrupt handler to the SERCOM

extern "C" {

  void SERCOM3_Handler(void);

  void SERCOM3_Handler(void) {

    myWire.onService();

  }
}
