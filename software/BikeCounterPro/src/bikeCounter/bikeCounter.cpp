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
        err = setup();
        currentStatus = (err) ? Status::errorState : Status::initSleep;
        break;

    case Status::initSleep:
        // RTC bug prevention
        // If the device runs on battery the rtc seems to reinitialize it's register after the first sleep period.
        // To avoid this a sleep is triggered in the first loop and the rtc time will be reset after waking up.
        currentStatus = Status::firstWakeUp;
        sleep(2000);
        break;

    case Status::firstWakeUp:
        logger.push("First wake-up");
        logger.loop();
        hal->rtcSetEpoch(defaultRTCEpoch);
        logger.push("RTC reset");
        logger.loop();
        currentStatus = Status::timeSync;
        logger.push("Time sync");
        logger.loop();
        break;

    case Status::timeSync:
        // while rtc time < defaultRTCEpoch + 1 month try to sync
        if (hal->rtcGetEpoch() < (defaultRTCEpoch + 2678400ul))
        {
            switch (timeSyncStat)
            {
            case 0:
                sendUplinkMessage();
                timeSyncStat = 1;
                break;
            case 1:
                if (!waitForLoRaModule())
                {
                    timeSyncStat = 2;
                }
                break;
            case 2:
                timeSyncStat = 0;
                sleep(syncTimeInterval * 1000UL, true);
                break;
            }
        }
        else
        {
            currentStatus = Status::collectData;
        }
        break;

    case Status::collectData:
    {
        // enable the PIR sensor
        hal->digitalWrite(pirPowerPin, 1);

        switch (processInput())
        {
        case 0:
        {
            std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds> currentTime{std::chrono::seconds{hal->rtcGetEpoch()}};
            sleep(getRemainingSleepTime(currentTime));
        }
        break;
        case 1:
            currentStatus = Status::sendPackage;
            break;
        case 2:
            currentStatus = Status::errorState;
            break;
        }
    }
    break;

    case Status::sendPackage:
        err = sendUplinkMessage();
        currentStatus = (err) ? Status::errorState : Status::waitForLoRa;
        break;

    case Status::waitForLoRa:
    {
        switch (waitForLoRaModule())
        {
        case 0:
        {
            currentStatus = Status::collectData;
            std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds> currentTime{std::chrono::seconds{hal->rtcGetEpoch()}};
            dataHandler.setTimerInterval(timeHandler.getCurrentIntervalMinutes(currentTime));
            sleep(getRemainingSleepTime(currentTime));
        }
        break;
        case 1:
            // wait
            break;
        case 2:
            currentStatus = Status::errorState;
            break;
        case 3:
            break;
        }
    }
    break;

    case Status::sleepState:
        if (!debugFlag || (debugFlag && (hal->getMillis() > sleepEndMillis)) || (motionDetected && !(preSleepStatus == Status::timeSync)))
        {
            currentStatus = preSleepStatus;
        }
        break;

    case Status::errorState:
        handleError();
        break;

    default:
        break;
    }
    hal->waitHere(50);
}

void BikeCounter::reset()
{
}

int BikeCounter::setup()
{
    // set static fields
    motionDetected = false;

    // read dip switch states
    hal->pinMode(switchPowerPin, HAL::GPIOPinMode::OUTPUT);
    hal->digitalWrite(switchPowerPin, 1);
    hal->waitHere(100);
    hal->pinMode(debugSwitchPin, HAL::GPIOPinMode::INPUT);
    hal->pinMode(configSwitchPin, HAL::GPIOPinMode::INPUT);
    debugFlag = hal->digitalRead(debugSwitchPin);
    configFlag = hal->digitalRead(configSwitchPin);
    hal->digitalWrite(switchPowerPin, 0);
    hal->waitHere(100);

    // deactivate the dip switch pins
    hal->pinMode(debugSwitchPin, HAL::GPIOPinMode::OUTPUT);
    hal->pinMode(configSwitchPin, HAL::GPIOPinMode::OUTPUT);
    hal->digitalWrite(debugSwitchPin, 0);
    hal->digitalWrite(configSwitchPin, 0);

    // setup pin modes
    hal->pinMode(ledPin, HAL::GPIOPinMode::OUTPUT);
    hal->pinMode(pirPowerPin, HAL::GPIOPinMode::OUTPUT);
    disableUnusedPins();
    blinkLED(2);

    // disable the pir sensor
    hal->digitalWrite(pirPowerPin, 0);

    hal->setAnalogReference();

    // initialize the I2C communication
    hal->I2CInit();

    // initialize the logging instance
    StatusLogger::Output outputType = debugFlag ? StatusLogger::Output::toSerial : StatusLogger::Output::noOutput;
    StatusLogger::getInstance()->setup(outputType, hal);

    // load config from flash
    logger.push("Load config from flash");
    logger.loop();
    std::string appEui = "";
    std::string appKey = "";
    if (hal->getEuiAndKeyFromFlash(&appEui, &appKey))
    {
        errorId = 1;
        return 1;
    }
    logger.push("appEui = " + appEui);
    logger.push("appKey = " + appKey);
    logger.push("Temp. sensor setup started");
    logger.loop();

    hal->waitHere(500);

    // initialize temperature and humidity sensor
    hal->AM2320Init();

    logger.push("Temp. sensor setup finished");
    logger.push("Lora setup started");
    logger.loop();

    hal->waitHere(500);

    // connect to lora network
    loRaConnector->injectHal(hal);
    loRaConnector->setup(appEui, appKey, &processDownlinkMessage);

    logger.push("Lora setup finished");
    logger.push("RTC setup started");
    logger.loop();

    // setup rtc
    hal->rtcBegin(true);
    hal->rtcSetEpoch(defaultRTCEpoch);

    logger.push("RTC current time: " +
                std::to_string(hal->rtcGetHours()) +
                ':' +
                std::to_string(hal->rtcGetMinutes()) +
                ':' +
                std::to_string(hal->rtcGetSeconds()));
    logger.push("RTC current date: " +
                std::to_string(hal->rtcGetDay()) +
                '.' +
                std::to_string(hal->rtcGetMonth()) +
                '.' +
                std::to_string(hal->rtcGetYear()));
    logger.push("RTC epoch: " +
                std::to_string(hal->rtcGetEpoch()));
    logger.loop();

    // delay to avoid interference with interrupt pin setup
    hal->waitHere(200);

    // setup counter interrupt
    hal->pinMode(counterInterruptPin, HAL::GPIOPinMode::INPUT);
    hal->attachInterruptWakeup(counterInterruptPin, onMotionDetected, HAL::TriggerMode::RISING);

    logger.push("Setup finished");
    logger.loop();

    return 0;
}

/// @brief
/// @return 0=no action; 1=send package 2=error
int BikeCounter::processInput()
{
    // get current time
    std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds> currentTime{std::chrono::seconds{hal->rtcGetEpoch()}};
    date::hh_mm_ss<std::chrono::seconds> currentTime_hms = date::make_time(currentTime.time_since_epoch() - date::floor<date::days>(currentTime).time_since_epoch());
    int timerCalled = 0;

    // check if a motion was detected.
    if (motionDetected)
    {
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

        logger.push("Motion detected (current count = " +
                    std::to_string(counter) +
                    " / time: " +
                    std::to_string(static_cast<int>(currentTime_hms.hours().count())) +
                    ':' +
                    std::to_string(static_cast<int>(currentTime_hms.minutes().count())) +
                    ':' +
                    std::to_string(static_cast<int>(currentTime_hms.seconds().count())) +
                    ')');
        logger.loop();

        // check if the data should be sent.
        int currentThreshold = dataHandler.getMaxCount(timeHandler.getCurrentIntervalMinutes(currentTime));
        if (counter >= currentThreshold)
        {
            // check if the floating interrupt pin bug occurred
            // method 1: check if the totalCount exceeds the maxCount between the timer calls.
            // method 2: detect if the count goes up very quickly. (faster then the board is able to send)
            if ((totalCounter >= maxCount) || (counter > (currentThreshold + 10)))
            {
                errorId = 2;
                return 2;
            }

            return 1;
        }
    }
    else
    {
        // if no motion was detected it means that the timer caused the wakeup.
        logger.push("Timer called");
        logger.loop();
        totalCounter = 0;
        nextAlarm = timeHandler.getNextIntervalTime(currentTime);
        return 1;
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
    dataHandler.setTemperature(hal->AM2320ReadTemperature());
    dataHandler.setHumidity(hal->AM2320ReadHumidity());
    dataHandler.setHourOfTheDay(hourOfDay);
    dataHandler.setDeviceTime(hal->rtcGetEpoch());
    dataHandler.setTimeArray(timeArray);

    loRaConnector->loop(5);
    if (loRaConnector->getStatus() != LoRaConnector::Status::connected)
    {
        return 2;
    }

    int err = loRaConnector->sendMessage(dataHandler.getPayload(), dataHandler.getPayloadLength());

    if (!err)
    {
        logger.push("Message enqueued for transmission! (count = " +
                    std::to_string(counter) +
                    " / temperature = " +
                    std::to_string(dataHandler.getTemperature()) +
                    "°C / humidity = " +
                    std::to_string(dataHandler.getHumidity()) +
                    "% / battery voltage = " +
                    std::to_string(dataHandler.getBatteryVoltage()) +
                    " V / DeviceEpoch = " +
                    std::to_string(dataHandler.getDeviceTime()) +
                    " )");
        logger.loop();

        // reset counter and time array
        counter = 0;

        for (int i = 0; i < timeArraySize; ++i)
        {
            timeArray[i] = 0;
        }
    }
    else
    {
        errorId = 4;
    }

    return err;
}

/// @brief
/// @return 0=connected, 1=busy, 2=error, 3=fatalError
int BikeCounter::waitForLoRaModule()
{
    loRaConnector->loop(10);

    switch (loRaConnector->getStatus())
    {
    case LoRaConnector::Status::connected:
        return 0;
    case LoRaConnector::Status::error:
        return 2;
    case LoRaConnector::Status::fatalError:
        loRaConnector->reset();
        return 3;
    default:
        return 1;
    }
}

void BikeCounter::blinkLED(int times, int mode)
{
    // deactivate the onboard LED after the specified amount of blinks
    static int blinkCount = 0;
    if (blinkCount < maxBlinks)
    {
        ++blinkCount;

        for (int i = 0; i < times; i++)
        {
            hal->waitHere(100);

            switch (mode)
            {
            case 0:
                // 0=blink
                hal->analogWrite(ledPin, 255);
                hal->waitHere(100);
                break;

            case 3: // 3=pulsate

            case 1: // 1=fade-in

                for (int i = 0; i < 256; i += 15)
                {
                    hal->analogWrite(ledPin, i);
                    hal->waitHere(50);
                }
                if (mode == 1)
                {
                    break;
                }

            case 2: // 2=fade-out
                for (int i = 255; i >= 0; i -= 15)
                {
                    hal->analogWrite(ledPin, i);
                    hal->waitHere(50);
                }
                break;
            }

            hal->analogWrite(ledPin, 0);
        }
    }
    else
    {
        // disable LED pin
        hal->pinMode(ledPin, HAL::GPIOPinMode::OUTPUT);
        hal->digitalWrite(ledPin, 0);
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
            hal->pinMode(i, HAL::GPIOPinMode::OUTPUT);
            hal->digitalWrite(i, 0);
        }
    }
}

float BikeCounter::getBatteryVoltage()
{
    // read the analog value and calculate the voltage
    return hal->analogRead(batteryVoltagePin) * 3.3f / 1023.0f / 1.2f * (1.2f + 0.33f);
}

void BikeCounter::sleep(int ms, bool noInterrupt)
{

    logger.push("Going to sleep for " + std::to_string(ms) + "ms (" + std::to_string((int)(ms / 1000)) + "s / " + std::to_string((int)(ms / 60000)) + "min)");
    logger.loop();

    preSleepStatus = currentStatus;
    currentStatus = Status::sleepState;
    motionDetected = false;

    if (noInterrupt)
    {
        // enable the PIR sensor
        hal->digitalWrite(pirPowerPin, 0);
    }

    if (debugFlag)
    {
        sleepEndMillis = hal->getMillis() + ms;
    }
    else
    {
        hal->deepSleep(ms);
    }
}

void BikeCounter::handleError()
{
    logger.push(errorMsg[errorId]);
    logger.loop();
    recErr = true;

    switch (errorId)
    {
    case 0:
        // Somehow we landed in the error state with no error pending.
        // This should never happen so we sleep for an hour and restart the device.
        currentStatus = Status::setupStep;
        sleep(60UL * 60UL * 1000UL, true);
        break;

    case 1:
        // The SPI Flash memory chip could not be initialized hence we cant read the configuration data.
        // Lets sleep for an hour and try again.
        currentStatus = Status::setupStep;
        sleep(60UL * 60UL * 1000UL, true);
        break;

    case 2:
        // The floating interrupt pin error was detected
        // Lets reset the PIR power connection and try again
        if (pirError++ <= 2)
        {
            logger.push("Resetting PIR-sensor");
            logger.loop();
            hal->digitalWrite(pirPowerPin, 0);
            hal->waitHere(2000);
            hal->digitalWrite(pirPowerPin, 1);
            totalCounter = 0;
            currentStatus = collectData;
            sleep(10UL * 60UL * 1000UL, true); // 10min
        }
        else
        {
            logger.push("PIR-sensor error could not be fixed.");
            logger.loop();
            // shut down PIR and try it again in 5 hours
            hal->digitalWrite(pirPowerPin, 0);
            errorId = 3;
            sleep(5UL * 60UL * 60UL * 1000UL, true); // 5h
        }
        break;

    case 3:
        // Restart the PIR and hope that the floating pin error disappeared
        hal->digitalWrite(pirPowerPin, 1);
        totalCounter = 0;
        currentStatus = collectData;
        sleep(60UL * 1000UL); // 1min

    case 4:
        // Reset the LoRa module or/and wait some time
        switch (loRaConnector->getErrorId())
        {
        case 1:
            // failed to start the module
        case 2:
            // Failed to connect to LoRa network
            // wait for 60min and try again
            // loRaConnector->reset();
            currentStatus = Status::collectData;
            sleep(60UL * 60UL * 1000UL, true);
            break;
        case 3:
            // Error sending message
            // wait for 5min and try again
            currentStatus = Status::collectData;
            sleep(5UL * 60UL * 1000UL, true);
            break;
        default:
            // loRaConnector->reset();
            currentStatus = Status::collectData;
            // enable the PIR sensor
            hal->digitalWrite(pirPowerPin, 1);
            break;
        }
        break;
    }
}

unsigned long BikeCounter::getRemainingSleepTime(std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds> currentTime)
{
    // determine the remaining time to sleep
    int64_t sdt = (nextAlarm.time_since_epoch().count() - currentTime.time_since_epoch().count());
    uint32_t sleepTime = sdt > 0 ? (uint32_t)sdt : syncTimeInterval;
    // sanity check
    sleepTime = std::min(sleepTime, (12ul * 60ul * 60ul));
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
    logger.push("Received time correction = " + std::to_string(timeDrift));
    logger.loop();

    uint32_t currentEpoch = hal->rtcGetEpoch();

    // check if the timeDrift should be applied (only if the drift is greater then 10min and only once a day)
    if ((abs(timeDrift) > (10 * 60)) && ((currentEpoch - lastRTCCorrection) > (24 * 60 * 60)))
    {
        // apply time correction
        hal->rtcSetEpoch(currentEpoch + timeDrift);
        lastRTCCorrection = hal->rtcGetEpoch();

        logger.push("RTC correction applied, current time: " +
                    std::to_string(hal->rtcGetHours()) +
                    ':' +
                    std::to_string(hal->rtcGetMinutes()) +
                    ':' +
                    std::to_string(hal->rtcGetSeconds()));
        logger.push("RTC current date: " +
                    std::to_string(hal->rtcGetDay()) +
                    '.' +
                    std::to_string(hal->rtcGetMonth()) +
                    '.' +
                    std::to_string(hal->rtcGetYear()));
        logger.push("RTC epoch: " +
                    std::to_string(hal->rtcGetEpoch()));
        logger.loop();

        hal->waitHere(500);
    }
}