#include "ArduinoLowPower.h"
#include <MKRWAN.h>

// set this to activate serial debug messages and to disable deepSleep
const bool debugFlag = 1;

// Threshold for non periodic data transmission
const int sendThreshold = 10;

// Interrupt pins
const int counterInterruptPin = 0;
const int timerInterruptPin = 1;

// Timer feedback pin
const int donePin = 4;

// lora modem object and application properties
LoRaModem modem(Serial1);
String appEui = "0000000000000000";
String appKey = "";

// Motion counter value
// must be volatile as incremented in interrupt
volatile int counter = 0;
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

  // setup timer feedback
  pinMode(donePin, OUTPUT);
  digitalWrite(donePin, LOW);

  if (debugFlag) {
    // open serial connection and wait
    Serial.begin(9600);
    while (!Serial);
  }

  // connect to lora network
  doConnect();

  // setup counter interrupt
  pinMode(counterInterruptPin, INPUT);
  LowPower.attachInterruptWakeup(counterInterruptPin, onMotionDetected, RISING);

  // setup timer interrupt
  pinMode(timerInterruptPin, INPUT);
  LowPower.attachInterruptWakeup(timerInterruptPin, onTimerInterrupt, RISING);

  delay(200);

  // clear timer request if allready occurred
  resetTimer();
}

void loop() {
  
  if (((counter >= sendThreshold) || (timerCalled)) && (!isSending)) {
    blinkLED(2);
    sendData();
  }

  if (!debugFlag) {
    delay(200);
    LowPower.sleep();
  }
}

void onMotionDetected() {
  if (!isSending) {
    counter++;
    blinkLED();
    if (debugFlag) {
      Serial.print("Motion detected (current count = ");
      Serial.print(counter);
      Serial.println(")");
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
}

void sendData() {
  isSending = 1;
  int err;
  //data is transmitted as Ascii chars
  modem.beginPacket();
  byte payload[1];
  payload[0] = lowByte(counter);
  modem.write(payload, sizeof(payload));
  err = modem.endPacket(false); // true = confirmation request
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

  // reset the timer
  resetTimer();

  // wait for all data transmission to finish
  delay(200);
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

// resets the timer by giving the "done" feedback
// (Falling edge on done pin)
void resetTimer () {
  timerCalled = 0;
  // generate 3 falling edges to be sure the timer catches it
  for (int i = 0; i < 3; ++i) {
    digitalWrite(donePin, HIGH);
    digitalWrite(donePin, LOW);
  }
}
