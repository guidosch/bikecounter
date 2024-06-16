#include "hal_arduino.hpp"

HAL_Arduino *HAL_Arduino::instance{nullptr};
// Thread-save Singleton (not needed for Arduino)
// std::mutex HAL::mutex_;

HAL_Arduino *HAL_Arduino::getInstance()
{
    // Thread-save Singleton (not needed for Arduino)
    // std::lock_guard<std::mutex> lock(mutex_);
    if (instance == nullptr)
    {
        instance = new HAL_Arduino();
    }
    return instance;
}

bool HAL_Arduino::getEuiAndKeyFromFlash(std::string *appEui, std::string *appKey)
{
    // LORA reset pin declaration as output
    pinMode(LORA_RESET, OUTPUT);
    // turn off LORA module to not interrupt the flash communication
    digitalWrite(LORA_RESET, LOW);
    delay(500);
    // begin flash communication
    if (flash.begin(PIN_FLASH_CS, 2000000, SPI1) == false)
    {
        return 1;
    }

    // first byte in memory contains the length of the config array
    uint8_t readSize = flash.readByte(0);
    // read buffer
    uint8_t rBuffer[255];
    // read bytes from flash
    flash.readBlock(1, rBuffer, readSize);
    // cast buffer to string array
    std::string config = std::string((char *)rBuffer);
    // split and assign config string
    *appEui = config.substr(config.find(':') + 1, config.find(';'));
    *appKey = config.substr(config.find(';') + 1).substr(config.find(':') + 1);
    // digitalWrite(LORA_RESET, HIGH);
    
    return 0;
}