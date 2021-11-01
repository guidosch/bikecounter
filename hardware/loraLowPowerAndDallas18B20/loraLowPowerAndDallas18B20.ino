#include <MKRWAN.h>
#include "ArduinoLowPower.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#define LORA_DEBUG

#define ONE_WIRE_BUS 17
#define PRECISION 10

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress forwardSensor, returnSensor;

LoRaModem modem(Serial1);
volatile int alarm_source;

//reference to my application on https://console.thethingsnetwork.org/
String appEui = "*****"; //freecooling-monitor
String appKey = "****"; //t77-monitor

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
  float t_forward = 0;
  float t_return = 0;
  
  //read several times as sensor does not work after sleep
  for (int i = 0; i <= 4; i++) {
    t_forward = getTemp(forwardSensor);
    t_return = getTemp(returnSensor);
    delay(100);
  }

  if (isnan(t_forward) || isnan(t_return)) {
    Serial.println(F("Failed to read from sensors!"));
    return;
  }
  
  int err;
  char data[30];
  sprintf(data,"{\"forward_temp\":%.2f, \"return_temp\":%.2f}", t_forward,t_return);
  Serial.println(data);
  modem.beginPacket();
  byte buffer[2];
  modem.write(encode(t_forward, buffer),2);
  modem.write(encode(t_return, buffer),2);
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
  //max a small number of bytes every two minutes!
  LowPower.sleep((30 * 60 * 1000));
  
}

float getTemp(DeviceAddress sensor) {
  return sensors.getTempC(sensor);
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
  digitalRead(17);
  sensors.begin();
  
  if (!sensors.getAddress(forwardSensor, 0)) Serial.println("Unable to find address for Device 0");
  if (!sensors.getAddress(returnSensor, 1)) Serial.println("Unable to find address for Device 1");

  sensors.setResolution(forwardSensor, PRECISION);
  sensors.setResolution(returnSensor, PRECISION);
  delay(100);
  sensors.requestTemperatures();
  delay(100);
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
