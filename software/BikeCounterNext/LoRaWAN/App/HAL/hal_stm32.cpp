#include <HAL/hal_stm32.hpp>

HAL_STM32 *HAL_STM32::instance{nullptr};
// Thread-save Singleton (not needed for single core MCU)
// std::mutex HAL::mutex_;

HAL_STM32 *HAL_STM32::getInstance()
{
    // Thread-save Singleton (not needed for single core MCU)
    // std::lock_guard<std::mutex> lock(mutex_);
    if (instance == nullptr)
    {
        instance = new HAL_STM32();
    }
    return instance;
}

bool HAL_STM32::getEuiAndKeyFromFlash(std::string *appEui, std::string *appKey)
{
    return 0;
}
