#include "bikeCounter.hpp"

BikeCounter *BikeCounter::GetInstance()
{
    std::lock_guard<std::mutex> lock(mutex);
    if (instance == nullptr)
    {
        instance = new BikeCounter();
    }
    return instance;
}

void BikeCounter::loop()
{
    switch (currentStatus)
    {
    case Status::setupStep:
        setup();

        break;
    case Status::initSleep:
        break;
    case Status::connectToLoRa:
        break;
    case Status::collectData:
        break;
    case Status::sendPackage:
        break;
    case Status::waitForDownlink:
        break;
    case Status::adjustClock:
        break;
    case Status::error:

    default:
        break;
    }
}

void BikeCounter::reset()
{
}

int BikeCounter::setup()
{
    // set static fields
    isSending = 0;
    motionDetected = 0;

    // read dip switch states
    pinMode(debugSwitchPin, INPUT);
    pinMode(configSwitchPin, INPUT);
    debugFlag = digitalRead(debugSwitchPin);
    configFlag = digitalRead(configSwitchPin);

    // deactivate the dip switch pins
    pinMode(debugSwitchPin, OUTPUT);
    pinMode(configSwitchPin, OUTPUT);
    digitalWrite(debugSwitchPin, LOW);
    digitalWrite(configSwitchPin, LOW);

    // setup pin modes
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(pirPowerPin, OUTPUT);
    disableUnusedPins();
    blinkLED(2);

    // disable the pir sensor
    digitalWrite(pirPowerPin, LOW);

    // The MKR WAN 1310 3.3V reference voltage for battery measurements
    analogReference(AR_DEFAULT);

    // initialize the I2C communication
    Wire.begin();

    // initialize the logging instance
    StatusLogger::Output outputType = debugFlag ? StatusLogger::Output::toSerial : StatusLogger::Output::noOutput;
    logger.setup(outputType);

    // load config from flash
    logger.push("Load config from flash");
    logger.loop();
    // LORA reset pin declaration as output
    pinMode(LORA_RESET, OUTPUT);
    // turn off LORA module to not interrupt the flash communication
    digitalWrite(LORA_RESET, LOW);
    delay(500);
    // begin flash communication
    if (flash.begin(PIN_FLASH_CS, 2000000, SPI1) == false)
    {
        errorId = 1;
        currentStatus = error;
        return 1;
    }

    // first byte in memory contains the length of the config array
    uint8_t readSize = flash.readByte(0);
    // read buffer
    uint8_t rBuffer[255];
    // read bytes from flash
    flash.readBlock(1, rBuffer, readSize);
    // cast buffer to string array
    String config = String((char *)rBuffer);
    // split and assign config string
    String appEui = config.substring(config.indexOf(':') + 1, config.indexOf(';'));
    String appKey = config.substring(config.indexOf(';') + 1).substring(config.indexOf(':') + 1);
    // digitalWrite(LORA_RESET, HIGH);
    logger.push(String("appEui = ") + appEui);
    logger.push(String("appKey = ") + appKey);
    logger.push("Temp. sensor setup started");
    logger.loop();

    delay(500);

    // initialize temperature and humidity sensor
    am2320.begin();

    logger.push("Temp. sensor setup finished");
    logger.push("Lora setup started");
    logger.loop();

    delay(500);

    // connect to lora network
    loRaConnector.setup(appEui, appKey, logger);
    while (loRaConnector.getStatus() != LoRaConnector::Status::connected)
    {
        loRaConnector.loop();
        delay(100);
    }

    logger.push("Lora setup finished");
    logger.push("RTC setup started");
    logger.loop();

    // setup rtc
    rtc.begin(true);
    rtc.setEpoch(defaultRTCEpoch);

    logger.push(String("RTC current time: ") +
                String(rtc.getHours()) +
                String(':') +
                String(rtc.getMinutes()) +
                String(':') +
                String(rtc.getSeconds()));
    logger.push(String("RTC current date: ") +
                String(rtc.getDay()) +
                String('.') +
                String(rtc.getMonth()) +
                String('.') +
                String(rtc.getYear()));
    logger.push(String("RTC epoch: ") +
                String(rtc.getEpoch()));
    logger.loop();

    // delay to avoid interference with interrupt pin setup
    delay(200);

    // setup counter interrupt
    pinMode(counterInterruptPin, INPUT_PULLDOWN);
    LowPower.attachInterruptWakeup(counterInterruptPin, onMotionDetected, RISING);

    logger.push("Setup finished");
    logger.loop();

    return 0;
}

void BikeCounter::blinkLED(int times)
{
    // deactivate the onboard LED after the specified amount of blinks
    static int blinkCount = 0;
    if (blinkCount < maxBlinks)
    {
        ++blinkCount;
        for (int i = 0; i < times; i++)
        {
            delay(100);
            digitalWrite(LED_BUILTIN, HIGH);
            delay(100);
            digitalWrite(LED_BUILTIN, LOW);
        }
    }
}

void BikeCounter::disableUnusedPins()
{
    for (int i = 0; i < 22; ++i)
    {
        // check if the current pin occurs in the pin array
        bool isUsed = false;
        for (int j = 0; j < usedPinCount; ++j)
        {
            if (usedPins[j] == i)
            {
                isUsed = true;
            }
        }
        // if not, set the pin to output and low
        if (!isUsed)
        {
            pinMode(i, OUTPUT);
            digitalWrite(i, LOW);
        }
    }
}

float BikeCounter::getBatteryVoltage()
{
    // read the analog value and calculate the voltage
    return analogRead(batteryVoltagePin) * 3.3f / 1023.0f / 1.2f * (1.2f + 0.33f);
}