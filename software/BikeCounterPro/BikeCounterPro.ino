#include <MKRWAN.h>
#include <RTCZero.h>
#include <SPI.h>
#include <SparkFun_SPI_SerialFlash.h>
#include <ArduinoLowPower.h>
#include <Adafruit_AM2320.h>
#include "versionConfig.h"
#include "src/dataPackage/dataPackage.hpp"
#include "src/timerSchedule/timerSchedule.hpp"

// ----------------------------------------------------
// ------------- Configuration section ----------------
// ----------------------------------------------------

// Max. counts between timer calls (to detect a floating interrupt pin)
const int maxCount = 1000;
// deactivate the onboard LED after the specified amount of blinks
const int maxBlinks = 50;

// Interrupt pins
const int counterInterruptPin = 1;
const int debugSwitchPin = 8;
const int configSwitchPin = 9;
const int batteryVoltagePin = A0;
// PIR sensor power pin
const int pirPowerPin = 2;
// Used pins (not defined pins will be disabled to save power)
const int usedPins[] = {LED_BUILTIN, counterInterruptPin, debugSwitchPin, configSwitchPin, batteryVoltagePin, pirPowerPin};

// Debug sleep interval (ms)
const uint32_t debugSleepTime = 300000ul; // 5*60*1000 ms

// Sync time interval
const uint32_t syncTimeInterval = 120000ul; // 2*60*1000 ms

// ----------------------------------------------------
// -------------- Declaration section -----------------
// ----------------------------------------------------

// lora modem object and application properties
LoRaModem modem(Serial1);
String appEui;
String appKey;

// Internal RTC object
RTCZero rtc;

// Humidity and temperature sensor object
Adafruit_AM2320 am2320 = Adafruit_AM2320();

// DataPackage object to encode the payload
DataPackage dataHandler = DataPackage();

// TimerSchedule object to determine the next timer call
TimerSchedule timeHandler = TimerSchedule();

// Motion counter value (must be volatile as incremented in IRS)
int counter = 0;
// total counts between timer calls
int totalCounter = 0;
// motion detected flag
volatile bool motionDetected = 0;
// time array size
const int timeArraySize = 62;
// time array
unsigned int timeArray[timeArraySize];
// hour of the day for next package
unsigned int hourOfDay = 0;
// Lora data transmission flag
volatile bool isSending = 0;
// Error counter for connection
int errorCounter = 0;
// Error counter for pir-sensor
int pirError = 0;
// Holds the debug state of the dip switch
int debugFlag = 0;
// Holds the config state of the dip switch
int configFlag = 0;
// Last call of main loop in debug mode
unsigned long lastMillis = millis() - 10 * 60 * 1000;
// default startup date 01.01.2022 (1640995200)
uint32_t defaultRTCEpoch = 1640995200ul;
// Status (See DataPackage.xlsx)
enum status
{
  no_error,
  na1,
  na2,
  na3,
  na4,
  na5,
  na6,
  sync_call
};
enum status deviceStatus = sync_call;
// Time sync skip flag (Prevents that the time correction is applied multiple times due to network lag and multiple enqueued downlinks with the same timeDrift information)
int skipTimeSync = 0;
// SPI serial flash parameter
const byte PIN_FLASH_CS = 32;
// SPI serial flash object
SFE_SPI_FLASH flash;
// Flags to trigger the initial sleep period and reset the rtc (prevent rtc bug)
int firstLoop = 1;
int firstWakeUp = 1;

// Blink method prototype
void blinkLED(int times = 1);

// ----------------------------------------------------
// -------------- Setup section -----------------
// ----------------------------------------------------

void setup()
{
  // read dip switch states
  pinMode(debugSwitchPin, INPUT);
  pinMode(configSwitchPin, INPUT);
  debugFlag = digitalRead(debugSwitchPin);
  configFlag = digitalRead(configSwitchPin);
  // deactivate the dip switch pins
  pinMode(debugSwitchPin, OUTPUT);
  pinMode(configSwitchPin, OUTPUT);
  digitalWrite(debugSwitchPin, LOW);
  digitalWrite(configSwitchPin, LOW);

  // setup pin modes
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(pirPowerPin, OUTPUT);
  disableUnusedPins(usedPins, sizeof(usedPins) / sizeof(usedPins[0]));
  blinkLED(2);

  // disable the pir sensor
  digitalWrite(pirPowerPin, LOW);

  // The MKR WAN 1310 3.3V reference voltage for battery measurements
  analogReference(AR_DEFAULT);

  // initialize the I2C communication
  Wire.begin();

  if (debugFlag)
  {
    // open serial connection and wait
    Serial.begin(115200);
    while (!Serial)
    {
    };

    Serial.println("Load config from flash");
  }

  // load config from flash
  // LORA reset pin declaration as output
  pinMode(LORA_RESET, OUTPUT);
  // turn off LORA module to not interrupt the flash communication
  digitalWrite(LORA_RESET, LOW);
  delay(500);
  // begin flash communication
  if (flash.begin(PIN_FLASH_CS, 2000000, SPI1) == false)
  {
    if (debugFlag)
    {
      Serial.println(F("SPI Flash not detected"));
    }
    while (1)
      ;
  }
  // first byte in memory contains the length of the config array
  uint8_t readSize = flash.readByte(0);
  // read buffer
  uint8_t rBuffer[255];
  // read bytes from flash
  flash.readBlock(1, rBuffer, readSize);
  // cast buffer to string array
  String config = String((char *)rBuffer);
  // split and assign config string
  appEui = config.substring(config.indexOf(':') + 1, config.indexOf(';'));
  appKey = config.substring(config.indexOf(';') + 1).substring(config.indexOf(':') + 1);
  // digitalWrite(LORA_RESET, HIGH);
  if (debugFlag)
  {
    Serial.print("appEui = ");
    Serial.println(appEui);
    Serial.print("appKey = ");
    Serial.println(appKey);
    Serial.println("Temp. sensor setup started");
  }

  delay(500);

  // initialize temperature and humidity sensor
  am2320.begin();

  if (debugFlag)
  {
    Serial.println("Temp. sensor setup finished");
    Serial.println("Lora setup started");
  }

  delay(500);

  // connect to lora network
  doConnect();

  if (debugFlag)
  {
    Serial.println("Lora setup finished");
    Serial.println("RTC setup started");
  }

  // setup rtc
  rtc.begin(true);
  rtc.setEpoch(defaultRTCEpoch);

  if (debugFlag)
  {
    // DateTime currentTime = rtc.now();
    Serial.print("RTC current time: ");
    Serial.print(rtc.getHours(), DEC);
    Serial.print(':');
    Serial.print(rtc.getMinutes(), DEC);
    Serial.print(':');
    Serial.println(rtc.getSeconds(), DEC);
    Serial.print("RTC current date: ");
    Serial.print(rtc.getDay(), DEC);
    Serial.print('.');
    Serial.print(rtc.getMonth(), DEC);
    Serial.print('.');
    Serial.println(rtc.getYear(), DEC);
    Serial.print("RTC epoch: ");
    Serial.println(rtc.getEpoch(), DEC);
  }

  // delay to avoid interference with interrupt pin setup
  delay(200);

  // setup counter interrupt
  pinMode(counterInterruptPin, INPUT_PULLDOWN);
  LowPower.attachInterruptWakeup(counterInterruptPin, onMotionDetected, RISING);

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
  // This statement simulates the sleep/deepSleep method in debug mode.
  // The reason for not using the sleep or deepSleep method is, that it disables
  // the usb connection which leeds to problems with the serial monitor.
  if (debugFlag)
  {
    // Check if a motion was detected or the sleep time expired
    // (Implemented in a nested if-statement to give the compiler the opportunity
    // to remove the whole outer statement depending on the constexpr debugFlag.)
    uint32_t sleepTime = ((deviceStatus != sync_call) && !skipTimeSync) ? debugSleepTime : syncTimeInterval; // sync call interval
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
  time_t currentTime = time_t(rtc.getEpoch());
  tm *currentTimeStruct = gmtime(&currentTime);
  int timerCalled = 0;

  if (debugFlag)
  {
    Serial.print("Device status = ");
    Serial.println(deviceStatus);
  }

  // check if a motion was detected.
  if (motionDetected)
  {
    motionDetected = 0;

    // set hour of the day if this was the first call
    if (counter == 0)
    {
      hourOfDay = currentTimeStruct->tm_hour;
    }
    timeArray[counter] = (currentTimeStruct->tm_hour - hourOfDay) * 60 + currentTimeStruct->tm_min;

    ++counter;
    ++totalCounter;

    blinkLED();

    if (debugFlag)
    {
      Serial.print("Motion detected (current count = ");
      Serial.print(counter);
      Serial.print(" / time: ");
      Serial.print(currentTimeStruct->tm_hour, DEC);
      Serial.print(':');
      Serial.print(currentTimeStruct->tm_min, DEC);
      Serial.print(':');
      Serial.print(currentTimeStruct->tm_sec, DEC);
      Serial.println(')');
    }
  }
  else
  {
    // if no motion was detected it means that the timer caused the wakeup.
    timerCalled = 1;
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
      if (debugFlag)
      {
        Serial.println("Timer called");
      }
    }

    timerCalled = 0;

    // check if the floating interrupt pin bug occurred
    // method 1: check if the totalCount exceeds the maxCount between the timer calls.
    // method 2: detect if the count goes up very quickly. (faster then the board is able to send)
    if ((totalCounter >= maxCount) || (counter > (currentThreshold + 5)))
    {
      ++pirError;
      if (pirError > 2)
      {
        if (debugFlag)
        {
          Serial.println("PIR-sensor error could not be fixed.");
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

    delay(200);

    // enable the pir sensor
    if (deviceStatus != sync_call)
    {
      digitalWrite(pirPowerPin, HIGH);
    }

    delay(200);

    isSending = 0;
  }

  delay(200);

  if (!debugFlag)
  {
    // determine the sleep time if we're not in debug mode
    uint32_t sleepTime = syncTimeInterval;
    if ((deviceStatus != sync_call) && !skipTimeSync)
    {
      time_t nextAlarm = timeHandler.getNextIntervalTime(currentTime);
      sleepTime = difftime(nextAlarm, currentTime) * 1000;
    }
    LowPower.deepSleep(sleepTime);
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

// Connects to LoRa network
void doConnect()
{
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
  delay(4000); // increase up to 10s if connection does not work
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
}

void sendData()
{
  int err;
  // data is transmitted as Ascii chars
  modem.beginPacket();

  dataHandler.setStatus(deviceStatus);
  dataHandler.setHwVersion(hwVersion);
  dataHandler.setSwVersion(swVersion);
  dataHandler.setMotionCount(counter);
  dataHandler.setBatteryVoltage(getBatteryVoltage());
  dataHandler.setTemperature(am2320.readTemperature());
  dataHandler.setHumidity(am2320.readHumidity());
  dataHandler.setHourOfTheDay(hourOfDay);
  dataHandler.setDeviceTime(rtc.getEpoch());
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
      Serial.print(dataHandler.getTemperature());
      Serial.print("°C / humidity = ");
      Serial.print(dataHandler.getHumidity());
      Serial.print("% / battery voltage = ");
      Serial.print(dataHandler.getBatteryVoltage());
      Serial.print(" V / DeviceEpoch = ");
      Serial.print(dataHandler.getDeviceTime());
      Serial.println(" )");
      Serial.print("Payload = ");
      for (int i = 0; i < dataHandler.getPayloadLength(); ++i)
      {
        Serial.print(dataHandler.getPayload()[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
    }
    counter = 0;
    for (int i = 0; i < timeArraySize; ++i)
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

  // wait for all data transmission to finish (up- and downlink)
  delay(10000);
  // receive and decode downlink message
  if (modem.available())
  {
    int rcv[64] = {0};
    int i = 0;
    while (modem.available())
    {
      rcv[i++] = modem.read();
    }
    if (debugFlag)
    {
      Serial.print("Received: ");
      for (unsigned int j = 0; j < i; j++)
      {
        Serial.print(rcv[j] >> 4, HEX);
        Serial.print(rcv[j] & 0xF, HEX);
        Serial.print(" ");
      }
      Serial.println();
    }

    // decode time drift
    int32_t timeDrift = 0;
    timeDrift = rcv[3];
    timeDrift = (timeDrift << 8) | rcv[2];
    timeDrift = (timeDrift << 8) | rcv[1];
    timeDrift = (timeDrift << 8) | rcv[0];
    if (debugFlag)
    {
      Serial.print("Time drift = ");
      Serial.println(timeDrift);
    }

    // check if the timeDrift should be applied
    if ((abs(timeDrift) > (10 * 60)) && !skipTimeSync)
    {
      // apply time correction
      correctRTCTime(timeDrift);
      // skip the next downlinks to avoid multiple corrections with the same value (due to the network delay)
      skipTimeSync = 1;
      // change the status and power on the PIR sensor
      if (deviceStatus == sync_call)
      {
        deviceStatus = no_error;

        // power the pir sensor
        digitalWrite(pirPowerPin, HIGH);
      }
    }
  }
  else
  {
    // set the flag to start listening to the downlink messages again.
    skipTimeSync = 0;
    if (debugFlag)
    {
      Serial.println("No downlink massage received.");
    }
  }
}

// blinks the on board led
void blinkLED(int times)
{
  // deactivate the onboard LED after the specified amount of blinks
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
    // check if the current pin occurs in the pin array
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

// reads the analog value and calculates the battery voltage.
float getBatteryVoltage()
{
  // read the input on analog pin 0 (A1) and calculate the voltage
  return analogRead(batteryVoltagePin) * 3.3f / 1023.0f / 1.2f * (1.2f + 0.33f);
}

// apply the given correction to the rtc time
void correctRTCTime(int32_t delta)
{
  // get the current time
  uint32_t currentEpoch = rtc.getEpoch();
  // set the new time
  rtc.setEpoch(currentEpoch + delta);

  if (debugFlag)
  {
    Serial.print("RTC correction applied, current time: ");
    Serial.print(rtc.getHours(), DEC);
    Serial.print(':');
    Serial.print(rtc.getMinutes(), DEC);
    Serial.print(':');
    Serial.println(rtc.getSeconds(), DEC);
    Serial.print("RTC current date: ");
    Serial.print(rtc.getDay(), DEC);
    Serial.print('.');
    Serial.print(rtc.getMonth(), DEC);
    Serial.print('.');
    Serial.println(rtc.getYear(), DEC);
    Serial.print("RTC epoch: ");
    Serial.println(rtc.getEpoch(), DEC);
  }
  delay(500);
}