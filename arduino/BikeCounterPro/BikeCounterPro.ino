#include "ArduinoLowPower.h"
#include <MKRWAN.h>
#include "RTClib.h"

// set debugFlag = 1 to activate serial debug messages and to disable deepSleep
const bool debugFlag = 1;

// Threshold for non periodic data transmission
const int sendThreshold = 10;
// Overflow threshold to detect a failure of the motion sensor
const int countOverflow = 10;

// lora modem object and application properties
LoRaModem modem(Serial1);
String appEui = "0000000000000000";
String appKey = "73876F853F8CE2E254F663DAE40FD811";

// RTC object
RTC_DS3231 rtc;
// Alarm interval (days, hours, minutes, seconds)
TimeSpan alarmInterval = TimeSpan(0, 0, 1, 0);

// Interrupt pins
const int timerInterruptPin = 0;
const int counterInterruptPin = 1;

// Motion counter value
// must be volatile as incremented in interrupt
volatile int counter = 0;
// Timer interrupt falg
volatile bool timerCalled = 0;
// Lora data transmission flag
volatile bool isSending = 0;
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

  if (debugFlag)
  {
    Serial.println("RTC setup started");
  }

  // setup rtc
  bool rtcConnection = rtc.begin();

  if ((!rtcConnection) && debugFlag)
  {
    Serial.println("NO connection to RTC");
  }
  if (rtc.lostPower()) {
    if (debugFlag) {
      Serial.println("RTC lost power, time will be reset.");
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
    Serial.println("RTC setup finished");
    Serial.println("Lora setup started");
  }

  // connect to lora network
  doConnect();

  if (debugFlag)  {
    Serial.println("Lora setup finished");
  }

  delay(200);

  sendData();

  delay(200);

  // setup timer interrupt pin
  pinMode(timerInterruptPin, INPUT_PULLUP);
  LowPower.attachInterruptWakeup(timerInterruptPin, onTimerInterrupt, FALLING);

  // setup counter interrupt
  pinMode(counterInterruptPin, INPUT);
  LowPower.attachInterruptWakeup(counterInterruptPin, onMotionDetected, RISING);


  if (debugFlag)  {
    Serial.println("Setup finished");
  }
}

void loop() {

  if (((counter >= sendThreshold) || (timerCalled)) && (!isSending)) {
    timerCalled = 0;
    setAlarm(alarmInterval);

    // check if the threshold between two counter calls is exeeded
    if (counter > (sendThreshold + countOverflow))
    {
      if (debugFlag)
      {
        Serial.println("Counter failure. To much motion between timer calls detected!");
        while (1) {}
      }
    }

    blinkLED(2);
    sendData();
  }

  delay(200);

  if (!debugFlag) {
    LowPower.sleep();
  }
}

void onMotionDetected() {
  if (!isSending) {
    counter++;
    blinkLED();
    DateTime motionTime = rtc.now();
    if (debugFlag) {
      Serial.print("Motion detected (current count = ");
      Serial.print(counter);
      Serial.print(" / time: ");
      Serial.print(motionTime.hour(), DEC);
      Serial.print(':');
      Serial.print(motionTime.minute(), DEC);
      Serial.print(':');
      Serial.print(motionTime.second(), DEC);
      Serial.println(')');
    }
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

void doConnect() {
  isSending = 1;
  if (!modem.begin(EU868)) {
    if (debugFlag) {
      Serial.println("Failed to start module");
    }
    blinkLED(5);
    while (1) {}
  };
  if (debugFlag) {
    Serial.print("Your module version is: ");
    Serial.println(modem.version());
    Serial.print("Your device EUI is: ");
    Serial.println(modem.deviceEUI());
  }
  delay(4000); //increase up to 10s if connectio does not work
  int connected = modem.joinOTAA(appEui, appKey);
  if (!connected) {
    if (debugFlag) {
      Serial.println("Something went wrong; are you indoor? Move near a window and retry");
    }
    while (1) {
      blinkLED();
    }
  }
  modem.minPollInterval(60);
  blinkLED(3);

  // wait for all data transmission to finish
  delay(500);
  isSending = 0;
}

void sendData() {
  isSending = 1;
  int err;
  //data is transmitted as Ascii chars
  modem.beginPacket();
  byte payload[1];
  payload[0] = lowByte(counter);
  modem.write(payload, sizeof(payload));
  err = modem.endPacket(false);
  if (err > 0) {
    if (debugFlag) {
      Serial.println("Message sent correctly!");
    }
    counter = 0;
  } else {
    if (debugFlag) {
      Serial.println("Error sending message :(");
    }
    errorCounter++;
    if (errorCounter > 1) {
      digitalWrite(LORA_RESET, LOW);
      if (debugFlag) {
        Serial.println("Trying to reconnect");
      }
      doConnect();
    }
  }

  // wait for all data transmission to finish
  delay(500);
  isSending = 0;
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
