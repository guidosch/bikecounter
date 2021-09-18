#include <MKRWAN.h>
#include "ArduinoLowPower.h"
#include <DHT.h>
#include <DHT_U.h>
#include "DHT.h"


#define DHTPIN_ROOM 15
#define DHTPIN_MEASURE 16
#define DHTTYPE DHT22
#define LORA_DEBUG

DHT dht_room(DHTPIN_ROOM, DHTTYPE);
DHT dht_measure(DHTPIN_MEASURE, DHTTYPE);
LoRaModem modem(Serial1);
volatile int alarm_source;

//reference to my application on https://console.thethingsnetwork.org/
String appEui = "*****"; //adjust for each application
String appKey = "*****"; //adjust for each device
int errorCounter = 0;

void setup() {
  Serial.begin(115200);
  //while (!Serial); //IMPORTANT - uncomment if you want to work with IDE
  
  //may fixes the spurious messages
  LowPower.attachInterruptWakeup(RTC_ALARM_WAKEUP, alarmEvent0, CHANGE);
  
  doConnect();
  blinkLED();
}

void loop() {
  
  enableSensors();
  
  float h_room = dht_room.readHumidity();
  float t_room = dht_room.readTemperature();
  float h_measure = dht_measure.readHumidity();
  float t_measure = dht_measure.readTemperature();
  //float batt = analogRead(ADC_BATTERY); //does not work yet
  
  if (isnan(h_room) || isnan(t_room) || isnan(h_measure) || isnan(t_measure)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  
  int err;
  char data[30];
  sprintf(data,"{\"room\":{\"t\":%.1f,\"h\":%.1f}, \"measure\":{\"t\":%.1f,\"h\":%.1f}}", t_room,h_room,t_measure,h_measure);
  Serial.println(data);
  modem.beginPacket();
  byte buffer[2];
  modem.write(encode(t_room, buffer),2);
  modem.write(encode(h_room, buffer),2);
  modem.write(encode(t_measure, buffer),2);
  modem.write(encode(h_measure, buffer),2);
  err = modem.endPacket(false);
  if (err > 0) {
    Serial.println("Message sent correctly!");
  } else {
    Serial.println("Error sending message:Error: "+err);
    errorCounter++;
    if(errorCounter > 1){
      digitalWrite(LORA_RESET, LOW);
      Serial.println("Trying to reconnect");
      doConnect();
    }
  }
  //max one message every two minutes!
  LowPower.sleep((30 * 60 * 1000));
  
}



byte *encode(float val,byte *buffer) {
  int16_t value = round(val * 100);
  // Encode int as bytes
  buffer[0] = highByte(value);
  buffer[1] = lowByte(value);
  return buffer;
}

void doConnect(){
  if (!modem.begin(EU868)) {
    Serial.println("Failed to start module");
    while (1) {}
  };
  Serial.print("Your module version is: ");
  Serial.println(modem.version());
  Serial.print("Your device EUI is: ");
  Serial.println(modem.deviceEUI());
  
  delay(4000); //had problems connection with shorter delay
  int connected = modem.joinOTAA(appEui, appKey);
  if (!connected) {
    Serial.println("Something went wrong; are you indoor? Move near a window and retry");
    blinkLED();
    blinkLED();
    blinkLED();
    while (1) {}
  }
  modem.minPollInterval(60);
}

//must be done every time after LowPower.sleep otherwise data stays the same
void enableSensors(){
  digitalRead(15);
  digitalRead(16);
  dht_room.begin();
  dht_measure.begin();
}

void blinkLED(){
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
}

void alarmEvent0() {
  alarm_source = 0;
}
