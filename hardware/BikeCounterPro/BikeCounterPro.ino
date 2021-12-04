#include <MKRWAN.h>
#include "ArduinoLowPower.h"
#include "RTClib.h"
//#include "Adafruit_Sensor.h"
#include "Adafruit_AM2320.h"
#include "arduino_secrets.h"
#include "dataPackage.hpp"

// ----------------------------------------------------
// ------------- Configuration section ----------------
// ----------------------------------------------------

// Set this to activate serial debug messages and to disable deepSleep.
constexpr bool debugFlag = 1;
// The sleep or deepSleep method disables the usb connection which
// leeds to problems with the serial monitor.

// threshold for non periodic data transmission
// dependes on the timer intervall
// timer <= 1h -> sendThreshold max = 62
// timer <= 2h -> sendThreshold max = 53
// timer <= 4h -> sendThreshold max = 47 (selected)
// timer <= 8h -> sendThreshold max = 41
// timer <= 17h -> sendThreshold max = 37
const int sendThreshold = 10;
// Timer interval (days, hours, minutes, seconds)
TimeSpan alarmInterval = TimeSpan(0, 0, 5, 0);
// Max. counts between timer calls (to detect a floating interrupt pin)
const int maxCount = 1000;
// deactivate the onboard LED after the spezified amount of blinks
const int maxBlinks = 50;

// Interrupt pins
const int timerInterruptPin = 0;
const int counterInterruptPin = 1;
const int batteryVoltagePin = A0;
// PIR sensor power pin
const int pirPowerPin = 2;
// Used pins (not defined pins will be disabled to save power)
const int usedPins[] = {LED_BUILTIN, timerInterruptPin, counterInterruptPin, batteryVoltagePin, pirPowerPin};

// ----------------------------------------------------
// -------------- Declaration section -----------------
// ----------------------------------------------------

// lora modem object and application properties
LoRaModem modem(Serial1);
String appEui = SECRET_APPEUI;
String appKey = SECRET_APPKEY;

// RTC object
RTC_DS3231 rtc;

// Humidity and temperature sensor object
Adafruit_AM2320 am2320 = Adafruit_AM2320();

// DataPackage object to encode the payload
DataPackage dataHandler = DataPackage(alarmInterval.totalseconds() / 60);

// Motion counter value (must be volatile as incremented in IRS)
int counter = 0;
// total counts between timer calls
int totalCounter = 0;
// motion detected flag
volatile bool motionDetected = 0;
// time array
unsigned int timeArray[sendThreshold];
// hour of the day for next package
unsigned int houreOfDay = 0;
// Timer interrupt falg
volatile bool timerCalled = 0;
// Lora data transmission flag
volatile bool isSending = 0;
// Error counter for connection
int errorCounter = 0;
// Error counter for pir-sensor
int pirError = 0;

// Blink methode prototype
void blinkLED(int times = 1);

// ----------------------------------------------------
// -------------- Setup section -----------------
// ----------------------------------------------------

void setup()
{
  // setup pin modes
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(pirPowerPin, OUTPUT);
  disableUnusedPins(usedPins, sizeof(usedPins) / sizeof(usedPins[0]));

  // disable the pir sensor
  digitalWrite(pirPowerPin, LOW);

  // The MKR WAN 1310 3.3V reference voltage for battery measurements
  analogReference(AR_DEFAULT);

  // initialize the I2C communication
  Wire.begin();

  if (debugFlag)
  {
    // open serial connection and wait
    Serial.begin(9600);
    while (!Serial)
      ;
    Serial.println("RTC setup started");
  }

  // setup rtc
  bool rtcConnection = rtc.begin();

  if ((!rtcConnection) && debugFlag)
  {
    Serial.println("NO connection to RTC");
  }
  if (rtc.lostPower())
  {
    if (debugFlag)
    {
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
    Serial.println("Temp. sensor setup started");
  }

  // initialize temperature and humidity sensor
  am2320.begin();

  if (debugFlag)
  {
    Serial.println("Temp. sensor setup finished");
    Serial.println("Lora setup started");
  }

  // connect to lora network
  doConnect();

  if (debugFlag)
  {
    Serial.println("Lora setup finished");
  }

  // delay to avoide interference with interrupt pin setup
  delay(200);

  // setup timer interrupt pin
  pinMode(timerInterruptPin, INPUT_PULLUP);
  LowPower.attachInterruptWakeup(timerInterruptPin, onTimerInterrupt, FALLING);

  // setup counter interrupt
  pinMode(counterInterruptPin, INPUT_PULLDOWN);
  LowPower.attachInterruptWakeup(counterInterruptPin, onMotionDetected, RISING);

  // power the pir sensor
  digitalWrite(pirPowerPin, HIGH);

  if (debugFlag)
  {
    Serial.println("Setup finished");
  }
}

// ----------------------------------------------------
// ---------------- Main loop section -----------------
// ----------------------------------------------------

// The main loop gets executed after the device wakes up
// (caused by the timer or the motion detection interrupt)
void loop()
{
  // check if a motion was detected.
  if (motionDetected)
  {
    motionDetected = 0;

    // reinitialize the rtc connection
    // due to the voltage drop while sending the connection needs to be reinitialize
    bool rtcConnection = rtc.begin();
    if ((!rtcConnection) && debugFlag)
    {
      Serial.println("NO connection to RTC");
    }

    DateTime currentTime = rtc.now();

    if (counter == 0)
    {
      houreOfDay = currentTime.hour();
    }
    timeArray[counter] = (currentTime.hour() - houreOfDay) * 60 + currentTime.minute();

    ++counter;
    ++totalCounter;

    blinkLED();

    if (debugFlag)
    {
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

  if (((counter >= sendThreshold) || (timerCalled)) && (!isSending))
  {
    if (timerCalled)
    {
      totalCounter = 0;
      if (debugFlag)
      {
        Serial.println("Timer called");
      }
    }

    timerCalled = 0;
    setAlarm(alarmInterval);

    // check if the floating interrupt pin bug occured
    // method 1: check if the totalCount exceeds the maxCount inbetween the timer calls.
    // method 2: detect if the count goes up very qickly. (faster then the board is able to send)
    if ((totalCounter >= maxCount) || (counter > (sendThreshold + 5)))
    {
      ++pirError;
      if (pirError > 2)
      {
        if (debugFlag)
        {
          Serial.println("PIR-sensor error could not me fixed.");
        }
        while (1)
        {
        }
      }
      if (debugFlag)
      {
        Serial.println("Floating interrupt pin detected.");
        Serial.println("Resetting PIR-sensor");
      }
      digitalWrite(pirPowerPin, LOW);
      delay(2000);
      digitalWrite(pirPowerPin, HIGH);
      totalCounter = 0;
    }

    blinkLED(2);
    sendData();
  }

  delay(200);

  if (!debugFlag)
  {
    delay(200);
    LowPower.deepSleep();
  }
}

// ----------------------------------------------------
// --------- Methode implementation section -----------
// ----------------------------------------------------

void onMotionDetected()
{
  if (!isSending)
  {
    motionDetected = 1;
  }
}

void onTimerInterrupt()
{
  if (!isSending)
  {
    timerCalled = 1;
  }
}

// Connects to LoRa network
void doConnect()
{
  isSending = 1;
  if (!modem.begin(EU868))
  {
    if (debugFlag)
    {
      Serial.println("Failed to start module");
    }
    blinkLED(5);
    while (1)
    {
    }
  };
  if (debugFlag)
  {
    Serial.print("Your module version is: ");
    Serial.println(modem.version());
    Serial.print("Your device EUI is: ");
    Serial.println(modem.deviceEUI());
  }
  delay(4000); //increase up to 10s if connectio does not work
  int connected = modem.joinOTAA(appEui, appKey);
  if (!connected)
  {
    if (debugFlag)
    {
      Serial.println("Something went wrong; are you indoor? Move near a window and retry");
    }
    while (1)
    {
      blinkLED();
    }
  }
  modem.minPollInterval(60);
  blinkLED(3);

  // wait for all data transmission to finish
  delay(500);
  isSending = 0;
}

void sendData()
{
  isSending = 1;
  int err;
  //data is transmitted as Ascii chars
  modem.beginPacket();

  dataHandler.setStatus(0);
  dataHandler.setMotionCount(counter);
  dataHandler.setBatteryLevel(getBatteryVoltage());
  dataHandler.setTemperatur(am2320.readTemperature());
  dataHandler.setHumidity(am2320.readHumidity());
  dataHandler.setHoureOfTheDay(houreOfDay);
  dataHandler.setTimeArray(timeArray);

  modem.write(dataHandler.getPayload(), dataHandler.getPayloadLength());
  err = modem.endPacket(false);
  if (err > 0)
  {
    if (debugFlag)
    {
      Serial.print("Message sent correctly! (count = ");
      Serial.print(counter);
      Serial.print(" / temperature = ");
      Serial.print(dataHandler.getTemperatur());
      Serial.print("°C / humidity = ");
      Serial.print(dataHandler.getHumidity());
      Serial.print("% / battery level = ");
      Serial.print(dataHandler.getBatteryLevel());
      Serial.print(" % / ");
      Serial.print(getBatteryVoltage());
      Serial.println(" V)");
    }
    counter = 0;
    for (int i = 0; i < sendThreshold; ++i)
    {
      timeArray[i] = 0;
    }
  }
  else
  {
    if (debugFlag)
    {
      Serial.println("Error sending message :(");
    }
    errorCounter++;
    if (errorCounter > 1)
    {
      digitalWrite(LORA_RESET, LOW);
      if (debugFlag)
      {
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
void blinkLED(int times)
{
  // deactivate the onboard LED after the spezified amount of blinks
  static int blinkCount = 0;
  if (blinkCount < maxBlinks)
  {
    ++blinkCount;
    for (int i = 0; i < times; i++)
    {
      delay(100);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
      digitalWrite(LED_BUILTIN, LOW);
    }
  }
}

// Sets all the unused pins to a defined level
// Output and LOW
void disableUnusedPins(const int *const activePins, int size)
{
  for (int i = 0; i < 22; ++i)
  {
    // check if the current pin occures in the pin array
    bool isUsed = false;
    for (int j = 0; j < size; ++j)
    {
      if (activePins[j] == i)
      {
        isUsed = true;
      }
    }
    // if not, set the pin to output and low
    if (!isUsed)
    {
      pinMode(i, OUTPUT);
      digitalWrite(i, LOW);
    }
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
