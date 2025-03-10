#ifndef STATUSLOGGER_H
#define STATUSLOGGER_H

#include <queue>
#include <string>
#include "../hal/hal_interface.hpp"

class StatusLogger
{
public:
    // Singletons should not be cloneable and not be assignable.
    StatusLogger(StatusLogger &other) = delete;
    void operator=(const StatusLogger &) = delete;

    static StatusLogger *getInstance();

    enum Output
    {
        noOutput,
        toSerial,
        toMemory
    };

    void setup(Output ot, HAL *hal_ptr);
    void loop();
    void push(const std::string msg);

protected:
    StatusLogger() {}
    ~StatusLogger() {}

private:
    static StatusLogger *instance;
    // Thread-save Singleton (not needed for Arduino)
    // static std::mutex mutex_;

    HAL *hal;
    enum Status
    {
        notReady,
        ready,
        error
    };
    Status currentStatus = notReady;
    Output outputType = noOutput;
    std::queue<std::string> msgQueue;
};

#endif // STATUSLOGGER_H