#include "ArduinoLowPower.h"
#include <MKRWAN.h>
#include "RTClib.h"

// set debugFlag = 1 to activate serial debug messages and to disable deepSleep
const bool debugFlag = 1;

// pins
const int timerInterruptPin = 0;
const int counterInterruptPin = 1;
const int batteryVoltagePin = A0;

// Threshold for non periodic data transmission
const int sendThreshold = 10;

// lora modem object and application properties
LoRaModem modem(Serial1);
String appEui = "0000000000000000";
String appKey = "73876F853F8CE2E254F663DAE40FD811";

// RTC object
RTC_PCF8523 rtc;
// Alarm interval (days, hours, minutes, seconds)
TimeSpan timerInterval = TimeSpan(0, 0, 1, 0);

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

void setup()
{
  // setup onboard LED
  pinMode(LED_BUILTIN, OUTPUT);

  if (debugFlag)
  {
    // open serial connection and wait
    Serial.begin(9600);
    while (!Serial)
      ;
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

  if (!rtc.initialized() || rtc.lostPower())
  {
    if (debugFlag)
    {
      Serial.println("RTC is NOT initialized, let's set the time!");
    }
    // sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // set the RTC to a specific time (year, month, day, hour, minute, second)
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  rtc.start();

  // deconfigure old timers
  rtc.deconfigureAllTimers();

  // set new timer intervall
  // rtc.enableCountdownTimer(PCF8523_FrequencyHour, 24);    // 1 day
  // rtc.enableCountdownTimer(PCF8523_FrequencyMinute, 150); // 2.5 hours
  rtc.enableCountdownTimer(PCF8523_FrequencySecond, 30); // 30 seconds

  pinMode(timerInterruptPin, INPUT_PULLUP);
  LowPower.attachInterruptWakeup(timerInterruptPin, onTimerInterrupt, FALLING);

  if (debugFlag)
  {
    Serial.println("RTC setup finished");
    Serial.println("Lora setup started");
  }

  // connect to lora network
  doConnect();

  // setup counter interrupt
  pinMode(counterInterruptPin, INPUT);
  LowPower.attachInterruptWakeup(counterInterruptPin, onMotionDetected, RISING);

  if (debugFlag)
  {
    Serial.println("Lora setup finished");
  }

  delay(200);

  sendData();

  delay(200);

  // The MKR WAN 1310 3.3V reference voltage for battery measurements
  analogReference(AR_DEFAULT);

  if (debugFlag)
  {
    Serial.println("Setup finished");
  }
}

void loop()
{

  if (((counter >= sendThreshold) || (timerCalled)) && (!isSending))
  {
    blinkLED(2);
    sendData();
  }

  delay(200);

  if (!debugFlag)
  {
    LowPower.sleep();
  }
}

void onMotionDetected()
{
  if (!isSending)
  {
    counter++;
    blinkLED();
    DateTime motionTime = rtc.now();
    if (debugFlag)
    {
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

void onTimerInterrupt()
{
  if (!isSending)
  {
    timerCalled = 1;
    if (debugFlag)
    {
      Serial.println("Timer called");
    }
  }
}

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
  byte payload[1];
  payload[0] = lowByte(counter);
  modem.write(payload, sizeof(payload));
  err = modem.endPacket(false);
  if (err > 0)
  {
    if (debugFlag)
    {
      Serial.print("Message sent correctly! (count = ");
      Serial.print(counter);
      Serial.print(" / battery level = ");
      Serial.print(getBatteryLevel());
      Serial.println(" %)");
    }
    counter = 0;
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
  for (int i = 0; i <= times; i++)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);
    digitalWrite(LED_BUILTIN, LOW);
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