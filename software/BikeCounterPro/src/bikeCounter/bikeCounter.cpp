#include "bikeCounter.hpp"

constexpr bool deepSleepDebug = true;

BikeCounter *BikeCounter::instance{nullptr};
// Thread-save Singleton (not needed for Arduino)
// std::mutex BikeCounter::mutex_;

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
    int err = 0;
    switch (currentStatus)
    {
    case Status::setupStep:
        if constexpr (deepSleepDebug)
        {
            blinkLED(1, 0);
            blinkLED(1, 1);
        }
        err = setup();
        currentStatus = (err) ? Status::errorState : Status::initSleep;
        break;

    case Status::initSleep:
        if constexpr (deepSleepDebug)
        {
            blinkLED(2, 0);
            blinkLED(1, 1);
        }
        // RTC bug prevention
        // If the device runs on battery the rtc seems to reinitialize it's register after the first sleep period.
        // To avoid this a sleep is triggered in the first loop and the rtc time will be reset after waking up.
        currentStatus = Status::firstWakeUp;
        sleep(2000);
        break;

    case Status::firstWakeUp:
        if constexpr (deepSleepDebug)
        {
            blinkLED(3, 0);
            blinkLED(1, 1);
        }
        logger.push("First wake-up");
        logger.loop();
        rtc.setEpoch(defaultRTCEpoch);
        logger.push("RTC reset");
        logger.loop();
        currentStatus = Status::timeSync;
        logger.push("Time sync");
        logger.loop();
        break;

    case Status::timeSync:
        if constexpr (deepSleepDebug)
        {
            blinkLED(4, 0);
            blinkLED(1, 1);
        }
        // while rtc time < defaultRTCEpoch + 1 month try to sync
        if (rtc.getEpoch() < (defaultRTCEpoch + 2678400ul))
        {
            if constexpr (deepSleepDebug)
            {
                blinkLED(1, 0);
                blinkLED(2, 1);
            }
            switch (timeSyncStat)
            {
            case 0:
                if constexpr (deepSleepDebug)
                {
                    blinkLED(2, 0);
                    blinkLED(2, 1);
                }
                sendUplinkMessage();
                timeSyncStat = 1;
                break;
            case 1:
                if constexpr (deepSleepDebug)
                {
                    blinkLED(3, 0);
                    blinkLED(2, 1);
                }
                if (!waitForLoRaModule())
                {
                    timeSyncStat = 2;
                }
                break;
            case 2:
                if constexpr (deepSleepDebug)
                {
                    blinkLED(4, 0);
                    blinkLED(2, 1);
                }
                timeSyncStat = 0;
                sleep(syncTimeInterval * 1000UL, true);
                break;
            }
        }
        else
        {
            if constexpr (deepSleepDebug)
            {
                blinkLED(5, 0);
                blinkLED(2, 1);
            }
            currentStatus = Status::collectData;
        }
        break;

    case Status::collectData:
    {
        if constexpr (deepSleepDebug)
        {
            blinkLED(5, 0);
            blinkLED(1, 1);
        }

        // enable the PIR sensor
        digitalWrite(pirPowerPin, HIGH);

        switch (processInput())
        {
        case 0:
        {
            if constexpr (deepSleepDebug)
            {
                blinkLED(6, 0);
                blinkLED(2, 1);
            }
            std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds> currentTime{std::chrono::seconds{rtc.getEpoch()}};
            sleep(getRemainingSleepTime(currentTime));
        }
        break;
        case 1:
            if constexpr (deepSleepDebug)
            {
                blinkLED(7, 0);
                blinkLED(2, 1);
            }
            currentStatus = Status::sendPackage;
            break;
        case 2:
            if constexpr (deepSleepDebug)
            {
                blinkLED(8, 0);
                blinkLED(2, 1);
            }
            currentStatus = Status::errorState;
            break;
        }
    }
    break;

    case Status::sendPackage:
        if constexpr (deepSleepDebug)
        {
            blinkLED(6, 0);
            blinkLED(1, 1);
        }
        err = sendUplinkMessage();
        currentStatus = (err) ? Status::errorState : Status::waitForLoRa;
        break;

    case Status::waitForLoRa:
    {
        if constexpr (deepSleepDebug)
        {
            blinkLED(7, 0);
            blinkLED(1, 1);
        }
        switch (waitForLoRaModule())
        {
        case 0:
        {
            if constexpr (deepSleepDebug)
            {
                blinkLED(9, 0);
                blinkLED(2, 1);
            }
            currentStatus = Status::collectData;
            std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds> currentTime{std::chrono::seconds{rtc.getEpoch()}};
            sleep(getRemainingSleepTime(currentTime));
        }
        break;
        case 1:
            if constexpr (deepSleepDebug)
            {
                blinkLED(10, 0);
                blinkLED(2, 1);
            }
            // wait
            break;
        case 2:
            if constexpr (deepSleepDebug)
            {
                blinkLED(11, 0);
                blinkLED(2, 1);
            }
            currentStatus = Status::errorState;
            break;
        case 3:
            if constexpr (deepSleepDebug)
            {
                blinkLED(12, 0);
                blinkLED(2, 1);
            }
            break;
        }
    }
    break;

    case Status::sleepState:
        if constexpr (deepSleepDebug)
        {
            blinkLED(8, 0);
            blinkLED(1, 1);
        }
        if (!debugFlag || (debugFlag && (millis() > sleepEndMillis)) || (motionDetected && !(preSleepStatus == Status::timeSync)))
        {
            if constexpr (deepSleepDebug)
            {
                blinkLED(1, 0);
                blinkLED(1, 2);
            }
            currentStatus = preSleepStatus;
        }
        break;

    case Status::errorState:
        if constexpr (deepSleepDebug)
        {
            blinkLED(9, 0);
            blinkLED(1, 1);
        }
        handleError();
        break;

    default:
        if constexpr (deepSleepDebug)
        {
            blinkLED(10, 0);
            blinkLED(1, 1);
        }
        break;
    }
    delay(50);
}

void BikeCounter::reset()
{
    if constexpr (deepSleepDebug)
    {
        blinkLED(2, 0);
        blinkLED(1, 2);
    }
}

int BikeCounter::setup()
{
    // set static fields
    motionDetected = false;

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
    StatusLogger::getInstance()->setup(outputType);

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
    loRaConnector->setup(appEui, appKey, &processDownlinkMessage);

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
    pinMode(counterInterruptPin, INPUT);
    LowPower.attachInterruptWakeup(digitalPinToInterrupt(counterInterruptPin), onMotionDetected, RISING);

    logger.push("Setup finished");
    logger.loop();

    return 0;
}

/// @brief
/// @return 0=no action; 1=send package 2=error
int BikeCounter::processInput()
{

    if constexpr (deepSleepDebug)
    {
        blinkLED(3, 0);
        blinkLED(1, 2);
    }

    // get current time
    std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds> currentTime{std::chrono::seconds{rtc.getEpoch()}};
    date::hh_mm_ss<std::chrono::seconds> currentTime_hms = date::make_time(currentTime.time_since_epoch() - date::floor<date::days>(currentTime).time_since_epoch());
    int timerCalled = 0;

    // check if a motion was detected.
    if (motionDetected)
    {

        if constexpr (deepSleepDebug)
        {
            blinkLED(4, 0);
            blinkLED(1, 2);
        }

        motionDetected = false;

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
        if (counter >= currentThreshold)
        {

            if constexpr (deepSleepDebug)
            {
                blinkLED(5, 0);
                blinkLED(1, 2);
            }

            // check if the floating interrupt pin bug occurred
            // method 1: check if the totalCount exceeds the maxCount between the timer calls.
            // method 2: detect if the count goes up very quickly. (faster then the board is able to send)
            if ((totalCounter >= maxCount) || (counter > (currentThreshold + 10)))
            {
                if constexpr (deepSleepDebug)
                {
                    blinkLED(6, 0);
                    blinkLED(1, 2);
                }
                errorId = 2;
                return 2;
            }

            return 1;
        }
    }
    else
    {
        if constexpr (deepSleepDebug)
        {
            blinkLED(7, 0);
            blinkLED(1, 2);
        }
        // if no motion was detected it means that the timer caused the wakeup.
        logger.push("Timer called");
        logger.loop();
        totalCounter = 0;
        nextAlarm = timeHandler.getNextIntervalTime(currentTime);
        return 1;
    }

    if constexpr (deepSleepDebug)
    {
        blinkLED(8, 0);
        blinkLED(1, 2);
    }

    return 0;
}

int BikeCounter::sendUplinkMessage()
{
    blinkLED(2);

    uint8_t stat = (currentStatus == Status::timeSync ? 7 : recErr);
    recErr = false;

    dataHandler.setStatus(stat);
    dataHandler.setHwVersion(hwVersion);
    dataHandler.setSwVersion(swVersion);
    dataHandler.setMotionCount(counter);
    dataHandler.setBatteryVoltage(getBatteryVoltage());
    dataHandler.setTemperature(am2320.readTemperature());
    dataHandler.setHumidity(am2320.readHumidity());
    dataHandler.setHourOfTheDay(hourOfDay);
    dataHandler.setDeviceTime(rtc.getEpoch());
    dataHandler.setTimeArray(timeArray);

    loRaConnector->loop(5);
    if (loRaConnector->getStatus() != LoRaConnector::Status::connected)
    {
        return 2;
    }

    if constexpr (deepSleepDebug)
    {
        blinkLED(9, 0);
        blinkLED(1, 2);
    }

    int err = loRaConnector->sendMessage(dataHandler.getPayload(), dataHandler.getPayloadLength());

    if (!err)
    {
        logger.push(String("Message enqueued for transmission! (count = ") +
                    String(counter) +
                    String(" / temperature = ") +
                    String(dataHandler.getTemperature()) +
                    String("°C / humidity = ") +
                    String(dataHandler.getHumidity()) +
                    String("% / battery voltage = ") +
                    String(dataHandler.getBatteryVoltage()) +
                    String(" V / DeviceEpoch = ") +
                    String(dataHandler.getDeviceTime()) +
                    String(" )"));
        logger.loop();

        // reset counter and time array
        counter = 0;
        for (int i = 0; i < timeArraySize; ++i)
        {
            timeArray[i] = 0;
        }
        if constexpr (deepSleepDebug)
        {
            blinkLED(10, 0);
            blinkLED(1, 2);
        }
    }
    else
    {
        errorId = 4;
        if constexpr (deepSleepDebug)
        {
            blinkLED(11, 0);
            blinkLED(1, 2);
        }
    }

    if constexpr (deepSleepDebug)
    {
        blinkLED(12, 0);
        blinkLED(1, 2);
    }

    return err;
}

/// @brief
/// @return 0=connected, 1=busy, 2=error, 3=fatalError
int BikeCounter::waitForLoRaModule()
{
    loRaConnector->loop(10);

    if constexpr (deepSleepDebug)
    {
        blinkLED(1, 0);
        blinkLED(1, 3);
    }

    switch (loRaConnector->getStatus())
    {
    case LoRaConnector::Status::connected:
        if constexpr (deepSleepDebug)
        {
            blinkLED(2, 0);
            blinkLED(1, 3);
        }
        return 0;
    case LoRaConnector::Status::error:
        if constexpr (deepSleepDebug)
        {
            blinkLED(3, 0);
            blinkLED(1, 3);
        }
        return 2;
    case LoRaConnector::Status::fatalError:
        if constexpr (deepSleepDebug)
        {
            blinkLED(4, 0);
            blinkLED(1, 3);
        }
        loRaConnector->reset();
        return 3;
    default:
        if constexpr (deepSleepDebug)
        {
            blinkLED(5, 0);
            blinkLED(1, 3);
        }
        return 1;
    }
}

void BikeCounter::blinkLED(int times, int mode)
{
    // deactivate the onboard LED after the specified amount of blinks
    static int blinkCount = 0;
    if (blinkCount < maxBlinks)
    {
        // TODO: remove
        //++blinkCount;

        for (int i = 0; i < times; i++)
        {
            delay(100);

            switch (mode)
            {
            case 0:
                // 0=blink
                analogWrite(LED_BUILTIN, 255);
                delay(100);
                break;

            case 3: // 3=pulsate

            case 1: // 1=fade-in

                for (int i = 0; i < 256; i += 15)
                {
                    analogWrite(LED_BUILTIN, i);
                    delay(50);
                }
                if (mode == 1)
                {
                    break;
                }

            case 2: // 2=fade-out
                for (int i = 255; i >= 0; i -= 15)
                {
                    analogWrite(LED_BUILTIN, i);
                    delay(50);
                }
                break;
            }

            analogWrite(LED_BUILTIN, 0);
        }
    }
    else
    {
        // disable LED pin
        pinMode(LED_BUILTIN, OUTPUT);
        digitalWrite(LED_BUILTIN, LOW);
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
    if constexpr (deepSleepDebug)
    {
        blinkLED(6, 0);
        blinkLED(1, 3);
    }
    // read the analog value and calculate the voltage
    return analogRead(batteryVoltagePin) * 3.3f / 1023.0f / 1.2f * (1.2f + 0.33f);
}

void BikeCounter::sleep(int ms, bool noInterrupt)
{

    logger.push("Going to sleep for " + String(ms) + "ms (" + String((int)(ms / 1000)) + "s / " + String((int)(ms / 60000)) + "min)");
    logger.loop();

    preSleepStatus = currentStatus;
    currentStatus = Status::sleepState;
    motionDetected = false;

    if (noInterrupt)
    {
        // enable the PIR sensor
        digitalWrite(pirPowerPin, LOW);
    }

    if constexpr (deepSleepDebug)
    {
        blinkLED(7, 0);
        blinkLED(1, 3);
    }

    if (debugFlag)
    {
        sleepEndMillis = millis() + ms;
    }
    else
    {
        LowPower.deepSleep(ms);
    }
}

void BikeCounter::handleError()
{
    if constexpr (deepSleepDebug)
    {
        blinkLED(1, 0);
        blinkLED(2, 3);
    }

    logger.push(errorMsg[errorId]);
    logger.loop();
    recErr = true;

    switch (errorId)
    {
    case 0:
        if constexpr (deepSleepDebug)
        {
            blinkLED(2, 0);
            blinkLED(2, 3);
        }
        // Somehow we landed in the error state with no error pending.
        // This should never happen so we sleep for an hour and restart the device.
        currentStatus = Status::setupStep;
        sleep(60UL * 60UL * 1000UL, true);
        break;

    case 1:
        if constexpr (deepSleepDebug)
        {
            blinkLED(3, 0);
            blinkLED(2, 3);
        }
        // The SPI Flash memory chip could not be initialized hence we cant read the configuration data.
        // Lets sleep for an hour and try again.
        currentStatus = Status::setupStep;
        sleep(60UL * 60UL * 1000UL, true);
        break;

    case 2:
        if constexpr (deepSleepDebug)
        {
            blinkLED(4, 0);
            blinkLED(2, 3);
        }
        // The floating interrupt pin error was detected
        // Lets reset the PIR power connection and try again
        if (pirError++ <= 2)
        {
            logger.push("Resetting PIR-sensor");
            logger.loop();
            digitalWrite(pirPowerPin, LOW);
            delay(2000);
            digitalWrite(pirPowerPin, HIGH);
            totalCounter = 0;
            currentStatus = collectData;
            if constexpr (deepSleepDebug)
            {
                blinkLED(5, 0);
                blinkLED(2, 3);
            }
            sleep(10UL * 60UL * 1000UL, true); // 10min
        }
        else
        {
            logger.push("PIR-sensor error could not be fixed.");
            logger.loop();
            // shut down PIR and try it again in 5 hours
            digitalWrite(pirPowerPin, LOW);
            errorId = 3;
            if constexpr (deepSleepDebug)
            {
                blinkLED(6, 0);
                blinkLED(2, 3);
            }
            sleep(5UL * 60UL * 60UL * 1000UL, true); // 5h
        }
        break;

    case 3:
        // Restart the PIR and hope that the floating pin error disappeared
        digitalWrite(pirPowerPin, HIGH);
        totalCounter = 0;
        currentStatus = collectData;
        if constexpr (deepSleepDebug)
        {
            blinkLED(7, 0);
            blinkLED(2, 3);
        }
        sleep(60UL * 1000UL); // 1min

    case 4:
        // Reset the LoRa module or/and wait some time
        switch (loRaConnector->getErrorId())
        {
        case 1:
            if constexpr (deepSleepDebug)
            {
                blinkLED(8, 0);
                blinkLED(2, 3);
            }
            // failed to start the module
        case 2:
            // Failed to connect to LoRa network
            // wait for 60min and try again
            // loRaConnector->reset();
            currentStatus = Status::collectData;
            if constexpr (deepSleepDebug)
            {
                blinkLED(9, 0);
                blinkLED(2, 3);
            }
            sleep(60UL * 60UL * 1000UL, true);
            break;
        case 3:
            // Error sending message
            // wait for 5min and try again
            currentStatus = Status::collectData;
            if constexpr (deepSleepDebug)
            {
                blinkLED(10, 0);
                blinkLED(2, 3);
            }
            sleep(5UL * 60UL * 1000UL, true);
            break;
        default:
            // loRaConnector->reset();
            currentStatus = Status::collectData;
            // enable the PIR sensor
            digitalWrite(pirPowerPin, HIGH);
            if constexpr (deepSleepDebug)
            {
                blinkLED(11, 0);
                blinkLED(2, 3);
            }
            break;
        }
        break;
    }
}

unsigned long BikeCounter::getRemainingSleepTime(std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds> currentTime)
{
    if constexpr (deepSleepDebug)
    {
        blinkLED(1, 0);
        blinkLED(3, 1);
    }
    // determine the remaining time to sleep
    int64_t sdt = (nextAlarm.time_since_epoch().count() - currentTime.time_since_epoch().count());
    uint32_t sleepTime = sdt > 0 ? (uint32_t)sdt : syncTimeInterval;
    // sanity check
    sleepTime = Min(sleepTime, (12ul * 60ul * 60ul));
    if constexpr (deepSleepDebug)
    {
        blinkLED(2, 0);
        blinkLED(3, 1);
    }
    return sleepTime * 1000UL;
}

int BikeCounter::processDownlinkMessage(int *buffer, int length)
{
    // decode time drift
    int32_t timeDrift = 0;
    timeDrift = buffer[3];
    timeDrift = (timeDrift << 8) | buffer[2];
    timeDrift = (timeDrift << 8) | buffer[1];
    timeDrift = (timeDrift << 8) | buffer[0];

    BikeCounter::getInstance()->correctRTCTime(timeDrift);

    return 0;
}

void BikeCounter::correctRTCTime(int32_t timeDrift)
{
    if constexpr (deepSleepDebug)
    {
        blinkLED(3, 0);
        blinkLED(3, 1);
    }
    logger.push(String("Received time correction = ") + String(timeDrift));
    logger.loop();

    uint32_t currentEpoch = rtc.getEpoch();

    // check if the timeDrift should be applied (only if the drift is greater then 10min and only once a day)
    if ((abs(timeDrift) > (10 * 60)) && ((currentEpoch - lastRTCCorrection) > (24 * 60 * 60)))
    {
        // apply time correction
        rtc.setEpoch(currentEpoch + timeDrift);
        lastRTCCorrection = rtc.getEpoch();

        logger.push(String("RTC correction applied, current time: ") +
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

        delay(500);
        if constexpr (deepSleepDebug)
        {
            blinkLED(4, 0);
            blinkLED(3, 1);
        }
    }
}