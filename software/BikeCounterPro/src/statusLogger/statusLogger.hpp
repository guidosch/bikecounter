#ifndef STATUSLOGGER_H
#define STATUSLOGGER_H

#include <Arduino.h>
#include <queue>
#include <string>

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

    void setup(Output ot);
    void loop();
    void push(const std::string msg);

protected:
    StatusLogger() {}
    ~StatusLogger() {}

private:
    static StatusLogger *instance;
    // Thread-save Singleton (not needed for Arduino)
    // static std::mutex mutex_;

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