#include <MKRWAN.h>
#include "ArduinoLowPower.h"
#include "arduino_secrets.h"

// set this to activate serial debug messages and to disable deepSleep
const bool debugFlag = 0;

// Threshold for non periodic data transmission
const int sendThreshold = 10;
// Sleep intervall (ms)
const u_int32_t sleepTime = 60000ul; // 4h = 14400000ul (4*60*60*1000)

// Interrupt pins
const int counterInterruptPin = 1;

// lora modem object and application properties
LoRaModem modem(Serial1);
String appEui = SECRET_APPEUI;
String appKey = SECRET_APPKEY;

// Motion counter value
// must be volatile as incremented in interrupt
volatile int counter = 0;
// motion detected flag
volatile bool motionDetected = 0;
// Timer interrupt falg
volatile bool timerCalled = 0;
// Lora data transmission flag
bool isSending = 0;
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
    Serial.println("Lora setup started");
  }

  // connect to lora network
  doConnect();

  if (debugFlag)
  {
    Serial.println("Lora setup finished");
  }

  // setup counter interrupt
  pinMode(counterInterruptPin, INPUT);
  LowPower.attachInterruptWakeup(counterInterruptPin, onMotionDetected, RISING);
}

void loop()
{
  // check if a motion was detected. If not the sleep periode is expired
  if (motionDetected)
  {
    motionDetected = 0;
    counter++;

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
  }

  // send the data if the threshold or the periodic time intervall is exceeded
  if (((counter >= sendThreshold) || (timerCalled)) && (!isSending))
  {
    timerCalled = 0;
    blinkLED(2);
    sendData();
  }

  delay(200);
  LowPower.sleep(sleepTime);
}

void onMotionDetected()
{
  if (!isSending)
  {
    motionDetected = 1;
  }
}

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
  for (int i = 0; i <= times; i++)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);
    digitalWrite(LED_BUILTIN, LOW);
  }
}
