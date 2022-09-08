#include <SPI.h>
#include "arduino_secrets.h"
#include <SparkFun_SPI_SerialFlash.h>

const byte PIN_FLASH_CS = 32;
SFE_SPI_FLASH flash;

void setup()
{
    Serial.begin(115200);
    while (!Serial)
        ;
    Serial.println("Serial ok");
    delay(500);

    pinMode(LORA_RESET, OUTPUT);   // LORA reset pin declaration as output
    digitalWrite(LORA_RESET, LOW); // turn off LORA module
    Serial.println("LoRa Off.");
    delay(500);

    if (flash.begin(PIN_FLASH_CS, 2000000, SPI1) == false)
    {
        Serial.println(F("SPI Flash not detected. Check wiring. Maybe you need to pull up WP/IO2 and HOLD/IO3? Freezing..."));
        while (1)
            ;
    }

    Serial.println(F("SPI Flash detected"));

    sfe_flash_manufacturer_e mfgID = flash.getManufacturerID();
    if (mfgID != SFE_FLASH_MFG_UNKNOWN)
    {
        Serial.print(F("Manufacturer: "));
        Serial.println(flash.manufacturerIDString(mfgID));
    }
    else
    {
        uint8_t unknownID = flash.getRawManufacturerID(); // Read the raw manufacturer ID
        Serial.print(F("Unknown manufacturer ID: 0x"));
        if (unknownID < 0x10)
            Serial.print(F("0")); // Pad the zero
        Serial.println(unknownID, HEX);
    }

    Serial.print(F("Device ID: 0x"));
    Serial.println(flash.getDeviceID(), HEX);

    //
    Serial.println("Erasing entire chip");
    flash.erase();

    Serial.println("Writing config to flash");
    char wBuffer[256] = "appeui:";
    strcat(wBuffer, SECRET_APPEUI);
    strcat(wBuffer, ";appkey:");
    strcat(wBuffer, SECRET_APPKEY);
    uint8_t wBufferSize = (uint8_t)strlen(wBuffer) + 1;
    // write size to first byte
    flash.writeBlock(0, &wBufferSize, 1);
    // write eui and key to flash
    flash.writeBlock(1, (uint8_t *)wBuffer, wBufferSize);

    //
    Serial.println("Read config from flash");
    uint8_t readSize = flash.readByte(0);
    Serial.print(readSize);
    uint8_t rBuffer[255];
    flash.readBlock(1, rBuffer, readSize);
    String config = String((char *)rBuffer);
    Serial.println(config);

    String appEui = config.substring(config.indexOf(':') + 1, config.indexOf(';'));
    String appKey = config.substring(config.indexOf(';') + 1).substring(config.indexOf(':') + 1);
    Serial.print("appEui = ");
    Serial.println(appEui);
    Serial.print("appKey = ");
    Serial.println(appKey);
}

void loop()
{
}