#include "ArduinoLowPower.h"
#include <MKRWAN.h>

bool debugFlag = 1;

LoRaModem modem(Serial1);
String appEui = "0000000000000000";
String appKey = "73876F853F8CE2E254F663DAE40FD811";

// Error counter for connection
int errorCounter = 0;
//must be volatile as incremented in interrupt
volatile int counter = 0;
//
bool timerCalled = 0;
//
bool isSending = 0;

// Interrupt pins
const int counterInterruptPin = 0;
const int timerInterruptPin = 1;

// Feedback pin to timer
const int donePin = 4;

// Threshold for non periodic data transmission
const int sendThreshold = 10;

//
void blinkLED(int times = 1);

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(donePin, OUTPUT);
  digitalWrite(donePin, LOW);

  if (debugFlag) {
    Serial.begin(9600);
    while (!Serial);
  }

  doConnect();

  //
  //pinMode(counterInterruptPin, INPUT);
  pinMode(counterInterruptPin, INPUT_PULLUP);
  //digitalRead(counterInterruptPin);
  LowPower.attachInterruptWakeup(counterInterruptPin, onMotionDetected, RISING);

  //
  pinMode(timerInterruptPin, INPUT);
  //pinMode(timerInterruptPin, INPUT_PULLUP);
  LowPower.attachInterruptWakeup(timerInterruptPin, onTimerInterrupt, RISING);

  delay(200);

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

  //
  resetTimer();

  //
  delay(200);
  isSending = 0;
}

void blinkLED(int times) {
  for (int i = 0; i <= times; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);
    digitalWrite(LED_BUILTIN, LOW);
  }
}

void resetTimer () {
  timerCalled = 0;
  for (int i = 0; i < 3; ++i) {
    digitalWrite(donePin, HIGH);
    digitalWrite(donePin, LOW);
  }
}
