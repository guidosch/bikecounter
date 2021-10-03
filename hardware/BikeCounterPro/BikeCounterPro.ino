#include "ArduinoLowPower.h"
#include <MKRWAN.h>
#include "RTClib.h"
#include "arduino_secrets.h"

// set debugFlag = 1 to activate serial debug messages and to disable deepSleep
constexpr bool debugFlag = 1;

// pins
const int timerInterruptPin = 0;
const int counterInterruptPin = 1;
const int batteryVoltagePin = A0;

// threshold for non periodic data transmission
// dependes on the timer intervall
// timer <= 1h -> sendThreshold max = 65
// timer <= 2h -> sendThreshold max = 56
// timer <= 3h -> sendThreshold max = 49 (selected)
const int sendThreshold = 10;
// overflow threshold to detect a failure of the motion sensor
const int countOverflow = 10;

// lora modem object and application properties
LoRaModem modem(Serial1);
String appEui = SECRET_APPEUI;
String appKey = SECRET_APPKEY;

// RTC object
RTC_DS3231 rtc;
// Alarm interval (days, hours, minutes, seconds)
TimeSpan alarmInterval = TimeSpan(0, 0, 1, 0);


// Motion counter value
// must be volatile as incremented in interrupt
volatile int counter = 0;
// time array
volatile unsigned int timeArray[sendThreshold];
// hour of the day for next package
volatile unsigned int houreOfDay = 0;
// Timer interrupt falg
volatile bool timerCalled = 0;
// Lora data transmission flag
volatile bool isSending = 0;
// Error counter for connection
int errorCounter = 0;
// LoraPayload size (Count + BatteryLevel + timeArray)
const int timeValueSize = 8; // 1h = 6bit, 2h = 7bit, 3h = 8bit
const int payloadSize = 1 + (int)((((float)(3 + 5 + timeValueSize * sendThreshold)) / 8.0f) + 1);

// Blink methode prototype
void blinkLED(int times = 1);

void setup() {
  // setup onboard LED
  pinMode(LED_BUILTIN, OUTPUT);

  // The MKR WAN 1310 3.3V reference voltage for battery measurements
  analogReference(AR_DEFAULT);

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

  onMotionDetected();
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
    DateTime currentTime = rtc.now();

    if (counter == 0)
    {
      houreOfDay = currentTime.hour();
    }
    timeArray[counter] = (currentTime.hour() - houreOfDay) * 60 + currentTime.minute();

    counter++;
    blinkLED();

    if (debugFlag) {
      Serial.print("Motion detected (current count = ");
      Serial.print(counter);
      Serial.print(" / time: ");
      Serial.print(currentTime.hour(), DEC);
      Serial.print(':');
      Serial.print(currentTime.minute(), DEC);
      Serial.print(':');
      Serial.print(currentTime.second(), DEC);
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
  byte payload[payloadSize];

  for (int i = 0; i < payloadSize; ++i)
  {
    payload[i] = 0;
  }

  payload[0] = lowByte(counter);

  byte batteryLevel = parsBatLevel(getBatteryLevel());
  byte batAndHour;

  bitWrite(batAndHour, 0, bitRead(batteryLevel, 0));
  bitWrite(batAndHour, 1, bitRead(batteryLevel, 1));
  bitWrite(batAndHour, 2, bitRead(batteryLevel, 2));
  bitWrite(batAndHour, 3, bitRead(houreOfDay, 0));
  bitWrite(batAndHour, 4, bitRead(houreOfDay, 1));
  bitWrite(batAndHour, 5, bitRead(houreOfDay, 2));
  bitWrite(batAndHour, 6, bitRead(houreOfDay, 3));
  bitWrite(batAndHour, 7, bitRead(houreOfDay, 4));
  payload[1] = batAndHour;

  for (int i = 0; i < counter; ++i)
  {
    payload[i + 2] = timeArray[i];
  }

  modem.write(payload, sizeof(payload));
  err = modem.endPacket(false);
  if (err > 0) {
    if (debugFlag) {
      Serial.print("Message sent correctly! (count = ");
      Serial.print(counter);
      Serial.print(" / battery level = ");
      Serial.print(getBatteryLevel());
      Serial.print(" % / ");
      Serial.print(getBatteryVoltage());
      Serial.println(" V)");
    }
    counter = 0;
    for (int i = 0; i < sendThreshold; ++i)
    {
      timeArray[i] = 0;
    }

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

float getBatteryVoltage()
{
  // read the input on analog pin 0 (A1) and calculate the voltage
  return analogRead(batteryVoltagePin) * 3.3f / 1023.0f / 1.2f * (1.2f + 0.33f);
}

// get battery level [%]
float getBatteryLevel()
{
  float bV = getBatteryVoltage();
  // charch calculation
  // devided into two ranges >=3.8V and <3.8
  // curve parameters from pseudo invers matrix polynom
  if (bV >= 3.8f)
  {
    return -178.57f * bV * bV + 1569.6f * bV - 3342.0f;
  }
  else
  {
    return 1183.7f * bV * bV * bV * bV - 15843.0f * bV * bV * bV + 79461.0f * bV * bV - 177004.0f * bV + 147744.0f;
  }
}

// pars the battery level to a 3 bit indicator
// 0 = 0%
// 7 = 100%
byte parsBatLevel(float batteryLevel)
{
  //
  if (batteryLevel < 0.0f)
  {
    batteryLevel = 0.0f;
  }
  float fract = 100.0f / (2 * 2 * 2 - 1);
  float indicator = batteryLevel / fract;

  return (byte)(round(indicator));
}
