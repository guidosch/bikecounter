#include <MKRWAN.h>
#include "ArduinoLowPower.h"
#include "arduino_secrets.h"

// ----------------------------------------------------
// ------------- Configuration section ----------------
// ----------------------------------------------------

// Set this to activate serial debug messages and to disable deepSleep.
constexpr bool debugFlag = 1;
// The sleep or deepSleep method disables the usb connection which
// leeds to problems with the serial monitor.

// Threshold for non periodic data transmission
const int sendThreshold = 40;
// Sleep intervall (ms)
const u_int32_t sleepTime = 60000ul; // 4h = 14400000ul (4*60*60*1000)
// Max. counts between timer calls (to detect a floating interrupt pin)
const int maxCount = 1000;
// deactivate the onboard LED after the spezified amount of blinks
const int maxBlinks = 50;

// Interrupt pins
const int counterInterruptPin = 1;
// PIR sensor power pin
const int pirPowerPin = 2;
// Used pins (not defined pins will be disabled to save power)
const int usedPins[] = {LED_BUILTIN, counterInterruptPin, pirPowerPin};

// ----------------------------------------------------
// -------------- Declaration section -----------------
// ----------------------------------------------------

// lora modem object and application properties
LoRaModem modem(Serial1);
String appEui = SECRET_APPEUI;
String appKey = SECRET_APPKEY;

// Motion counter value (must be volatile as incremented in IRS)
volatile int counter = 0;
// total counts between timer calls
volatile int totalCounter = 0;
// motion detected flag
volatile bool motionDetected = 0;
// Timer interrupt falg
volatile bool timerCalled = 0;
// Lora data transmission flag
bool isSending = 0;
// Error counter for connection
int errorCounter = 0;
// Error counter for pir-sensor
int pirError = 0;
// Last call of main loop in debug mode
unsigned long lastMillis = 0;

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

  // power the pir sensor
  digitalWrite(pirPowerPin, HIGH);

  if (debugFlag)
  {
    // open serial connection and wait
    Serial.begin(9600);
    while (!Serial)
      ;
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

  // setup counter interrupt
  pinMode(counterInterruptPin, INPUT_PULLDOWN);
  LowPower.attachInterruptWakeup(counterInterruptPin, onMotionDetected, RISING);
}

// ----------------------------------------------------
// ---------------- Main loop section -----------------
// ----------------------------------------------------

// The main loop gets executed after the device wakes up
// (caused by the timer or the motion detection interrupt)
void loop()
{
  // This statement simulates the sleep/deepSleep methode in debug mode.
  // The reason for not using the sleep or deepSleep method is, that it disables
  // the usb connection which leeds to problems with the serial monitor.
  if (debugFlag)
  {
    // Check if a motion was detected or the sleep time expired
    // (Implemented in a nested if-statement to give the compiler the opportunity
    // to remove the whole outer statement depending on the constexpr debugFlag.)
    if ((motionDetected) || ((millis() - lastMillis) >= sleepTime))
    {
      // Run the main loop once
      lastMillis = millis();
    }
    else
    {
      // Noting happend, continue with the next cycle
      return;
    }
  }

  // check if a motion was detected. If not the sleep periode is expired
  if (motionDetected)
  {
    motionDetected = 0;
    ++counter;
    ++totalCounter;

    blinkLED();

    if (debugFlag)
    {
      Serial.print("Motion detected (current count = ");
      Serial.print(counter);
      Serial.println(')');
    }
  }
  else
  {
    timerCalled = 1;
    totalCounter = 0;
  }

  // check if the floating interrupt pin bug occured
  if ((totalCounter >= maxCount) || (counter > (sendThreshold + 10)))
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

  // send the data if the threshold or the periodic time intervall is exceeded
  if (((counter >= sendThreshold) || (timerCalled)) && (!isSending))
  {
    timerCalled = 0;
    blinkLED(2);
    sendData();
  }

  // Put the board to sleep
  if (!debugFlag)
  {
    delay(200);
    LowPower.deepSleep(sleepTime);
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

// Sends the acquired data
void sendData()
{
  isSending = 1;
  int err;
  //data is transmitted as Ascii chars
  modem.beginPacket();
  byte payload[1];
  payload[0] = lowByte(counter);
  modem.write(payload, sizeof(payload));
  err = modem.endPacket(false); // true = confirmation request
  if (err > 0)
  {
    if (debugFlag)
    {
      Serial.println("Message sent correctly!");
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
  delay(200);
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