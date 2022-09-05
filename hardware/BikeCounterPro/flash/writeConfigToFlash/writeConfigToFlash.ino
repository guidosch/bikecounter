#include <SerialFlash.h>
#include <SPI.h>
#include <MKRWAN.h>
#include "arduino_secrets.h"

const int FlashChipSelect = 32; // the 2MB flash CS pin is connected to pin 32
int FCstate = 8;                // to see the state of the function
char filename[] = "bikeCounterConfig";
uint32_t size = 256;

void setup()
{
    pinMode(LORA_RESET, OUTPUT);   // LORA reset pin declaration as output
    digitalWrite(LORA_RESET, LOW); // turn off LORA module
    Serial.begin(115200);
    while (!Serial)
        ;                        // wait for serial monitor to connect
    Serial.println("Serial ok"); // just a test to see the sketch has started
    delay(500);

    FCstate = SerialFlash.begin(SPI1, FlashChipSelect); // SerialFlash initialization on SPI1 bus, CS is on 32
    Serial.println(FCstate);                            // to see the return of the SerialFlash.begin function on serial
    if (FCstate == 1)
    {

        if (!SerialFlash.exists(filename))
        {
            Serial.println("File does not exist. (will be created)");
            SerialFlash.create(filename, size);
            if (SerialFlash.exists(filename))
            {
                Serial.println("File successfully created.");
            }
            else
            {
                Serial.println("Was not able to create the file. Will erase full flash and try again.");
                // SerialFlash.eraseAll();
                while (SerialFlash.ready() == false)
                {
                    // wait, 30 seconds to 2 minutes for most chips
                }
                SerialFlash.create(filename, size);
                if (SerialFlash.ready() == false)
                {
                    Serial.println("File successfully created.");
                }
                else
                {
                    Serial.println("Still not able to create the file!");
                    while (1)
                    {
                    }
                }
            }
        }

        SerialFlashFile file;
        file = SerialFlash.open(filename);
        if (file)
        {
            char wBuffer[size] = "appeui:SECRET_APPEUI;appkey:SECRET_APPKEY";
            file.write(wBuffer, size);
        }

        char rBuffer[size];
        file.read(rBuffer, size);
        Serial.print("Read from flash: ");
        for (int i = 0; i < size; ++i)
        {
            Serial.print(rBuffer[i]);
        }
        Serial.println("");

        /*
        uint32_t startAddr = 0;
        char wBuffer[size] = "appeui:SECRET_APPEUI;appkey:SECRET_APPKEY";
        SerialFlash.write(startAddr, wBuffer, size);

        char rBuffer[size] = "";
        SerialFlash.read(startAddr, rBuffer, size);
        Serial.print("Read from flash: ");
        for (int i = 0; i < size; ++i)
        {
            Serial.print(rBuffer[i]);
        }
        Serial.println("");
        */
    }

    Serial.println("Something went wrong with the flash!");
    delay(1500);
}

void loop()
{
}