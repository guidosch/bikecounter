#include "ArduinoLowPower.h"
#include <MKRWAN.h>

LoRaModem modem(Serial1);
//reference to my application on https://console.thethingsnetwork.org/
String appEui = "*****"; //adjust
String appKey = "****"; //antennentrail_2


int errorCounter = 0;
//must be volatile as incremented in interrupt
volatile int counter = 0;
//clock is stopped in low power mode, so we only can send after a v

// Pin used to trigger wakeup
const int pin = 17; // --> A2
const int SEND_THRESHOLD =40;

void setup() {
  Serial.begin(9600);
  
  //while (!Serial); //comment - if on battery!!!
  
  doConnect();
  
  pinMode(LED_BUILTIN, OUTPUT);
  digitalRead(pin);
  LowPower.attachInterruptWakeup(pin, motionCounter, RISING);
  delay(200);
}

void loop() {
  blinkLED();

  if (counter >= SEND_THRESHOLD) {
    int err;
    //data is transmitted as Ascii chars 
    modem.beginPacket();
    byte payload[1];
    payload[0] = lowByte(counter);
    modem.write(payload, sizeof(payload));
    err = modem.endPacket(false);
    if (err > 0) {
      Serial.println("Message sent correctly!");
      counter = 0;
    } else {
      Serial.println("Error sending message :(");
      errorCounter++;
      if(errorCounter > 1){
        digitalWrite(LORA_RESET, LOW);
        Serial.println("Trying to reconnect");
        doConnect();
      }
    }
  }
  LowPower.sleep();
}

void motionCounter() {
  counter++;
}

void doConnect() {
  if (!modem.begin(EU868)) {
    Serial.println("Failed to start module");
    blink(5);
    while (1) {}
  };
  Serial.print("Your module version is: ");
  Serial.println(modem.version());
  Serial.print("Your device EUI is: ");
  Serial.println(modem.deviceEUI());
  
  delay(4000); //increase up to 10s if connectio does not work
  int connected = modem.joinOTAA(appEui, appKey);
  if (!connected) {
    Serial.println("Something went wrong; are you indoor? Move near a window and retry");
    while (1) {
      blinkLED();
    }
  }
  modem.minPollInterval(60);
  blink(3);
}

void blink(int times){
  for (int i = 0; i <= times; i++) {
    blinkLED();
  }
}

void blinkLED() {
  Serial.println("Counter: "+counter);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(200);
  digitalWrite(LED_BUILTIN, LOW);
}