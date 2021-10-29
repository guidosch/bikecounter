#include "ArduinoLowPower.h"

const int sleepInterruptPin = 1;
volatile bool interruptDetected = 0;

void blinkLED(int times = 1);

void setup()
{
    // setup counter interrupt
    pinMode(sleepInterruptPin, INPUT_PULLUP);
    LowPower.attachInterruptWakeup(sleepInterruptPin, onSleepInterrupted, FALLING);
}

// blink 1 time = normal loop call
// blink 2 times = called by interrupt
void loop()
{
    blinkLED();

    if (interruptDetected)
    {
        interruptDetected = 0;
        delay(200);
        blinkLED();
    }

    LowPower.sleep(10000ul); // 4h = 14400000ul (4*60*60*1000)
}

void onSleepInterrupted()
{
    interruptDetected = 1;
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