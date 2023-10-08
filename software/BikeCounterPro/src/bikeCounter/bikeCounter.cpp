#include "bikeCounter.hpp"

BikeCounter *BikeCounter::getInstance()
{
    // Thread-save Singleton (not needed for Arduino)
    // std::lock_guard<std::mutex> lock(mutex_);
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
        int err = setup();
        currentStatus = (!err) ? Status::initSleep : Status::error;
        break;
    case Status::initSleep:
        // RTC bug prevention
        // If the device runs on battery the rtc seems to reinitialize it's register after the first sleep period.
        // To avoid this a sleep is triggered in the first loop and the rtc time will be reset after waking up.
        currentStatus = Status::firstWakeUp;
        sleep(2000);
        break;
    case Status::firstWakeUp:
        rtc.setEpoch(defaultRTCEpoch);
        currentStatus = Status::connectToLoRa;
        break;
    case Status::connectToLoRa:
        break;
    case Status::collectData:
        processInput();
        break;
    case Status::sendPackage:
        int err = sendUplinkMessage();
        break;
    case Status::waitForDownlink:
        break;
    case Status::adjustClock:
        break;
    case Status::sleep:
        if (!debugFlag || (debugFlag && (millis() > sleepEndMillis)))
        {
            currentStatus = preSleepStatus;
        }
        break;
    case Status::error:
        break;
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
    loRaConnector->setup(appEui, appKey, &logger, &processDownlinkMessage);

    // TODO: This is only temporary to keep the old connection order. This should be removed
    // and the connection should be attempted in the first normal loop.
    while (loRaConnector->getStatus() != LoRaConnector::Status::connected)
    {
        loRaConnector->loop();
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

int BikeCounter::processInput()
{
    // get current time
    std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds> currentTime{std::chrono::seconds{rtc.getEpoch()}};
    date::hh_mm_ss<std::chrono::seconds> currentTime_hms = date::make_time(currentTime.time_since_epoch() - date::floor<date::days>(currentTime).time_since_epoch());
    int timerCalled = 0;

    logger.push(String("Device status = ") +
                String(currentStatus));
    logger.loop();

    // check if a motion was detected.
    if (motionDetected)
    {
        motionDetected = 0;

        // set hour of the day if this was the first call
        if (counter == 0)
        {
            hourOfDay = currentTime_hms.hours().count();
        }
        timeArray[counter] = (currentTime_hms.hours().count() - hourOfDay) * 60 + currentTime_hms.minutes().count();

        ++counter;
        ++totalCounter;

        blinkLED();

        logger.push(String("Motion detected (current count = ") +
                    String(counter) +
                    String(" / time: ") +
                    String(static_cast<int>(currentTime_hms.hours().count())) +
                    String(':') +
                    String(static_cast<int>(currentTime_hms.minutes().count())) +
                    String(':') +
                    String(static_cast<int>(currentTime_hms.seconds().count())) +
                    String(')'));
        logger.loop();

        // check if the data should be sent.
        int currentThreshold = dataHandler.getMaxCount(timeHandler.getCurrentIntervalMinutes(currentTime));
        if (counter >= currentThreshold){
            currentStatus = Status::sendPackage;
        }
    }
    else
    {
        // if no motion was detected it means that the timer caused the wakeup.
        currentStatus = Status::sendPackage;
    }

    return 0;
}

int BikeCounter::sendUplinkMessage(){

    // TODO: Continue here
    return 0;
}

void BikeCounter::blinkLED(int times = 1)
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

void BikeCounter::sleep(int m)
{
    preSleepStatus = currentStatus;
    currentStatus = Status::sleep;
    if (debugFlag)
    {
        sleepEndMillis = millis() + m;
    }
    else
    {
        LowPower.deepSleep(m);
    }
}

int BikeCounter::processDownlinkMessage(int *buffer, int length)
{
    // TODO: Needs to be implemented
    return 0;
}