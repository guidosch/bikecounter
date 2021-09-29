#include "ArduinoLowPower.h"
#include <MKRWAN.h>
#include "Adafruit_Si7021.h"

// set debugFlag = 1 to activate serial debug messages and to disable deepSleep
const bool debugFlag = 1;

// Threshold for non periodic data transmission
const int sendThreshold = 10;

// lora modem object and application properties
LoRaModem modem(Serial1);
String appEui = "0000000000000000";
String appKey = "";

bool enableHeater = false;
uint8_t loopCnt = 0;

Adafruit_Si7021 sensor = Adafruit_Si7021();

// Interrupt pins
const int counterInterruptPin = 5;

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

  if (debugFlag) {    
    // open serial connection and wait
    Serial.begin(9600);
    while (!Serial);
  }

  Serial.println("Si7021 retup");
  
  if (!sensor.begin()) {
    Serial.println("Did not find Si7021 sensor!");
    while (true)
      ;
  }

  Serial.println("LoRa setup");

  // connect to lora network
  doConnect();

  // setup counter interrupt
  pinMode(counterInterruptPin, INPUT);
  LowPower.attachInterruptWakeup(counterInterruptPin, onMotionDetected, RISING);

  if (debugFlag)  {
    Serial.println("Lora setup finished");
  }

  delay(200);


}

void loop() {

  Serial.print("Humidity:    ");
  Serial.print(sensor.readHumidity(), 2);
  Serial.print("\tTemperature: ");
  Serial.println(sensor.readTemperature(), 2);
  delay(1000);

  if (((counter >= sendThreshold) || (timerCalled)) && (!isSending)) {
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
  delay(200);
  isSending = 0;
}

void sendData() {
  isSending = 1;
  Serial.println("1");
  int err;
  //data is transmitted as Ascii chars
  modem.beginPacket();
  Serial.println("2");
  byte payload[1];
  Serial.println("3");
  payload[0] = lowByte(counter);
  Serial.println("4");
  modem.write(payload, sizeof(payload));
  Serial.println("5");
  err = modem.endPacket(false);
  Serial.println("6");
  if (err > 0) {
    if (debugFlag) {
      Serial.println("Message sent correctly!");
    }
    Serial.println("7");
    counter = 0;
    Serial.println("8");
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
