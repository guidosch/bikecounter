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

