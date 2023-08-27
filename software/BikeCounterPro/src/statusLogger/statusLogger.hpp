#ifndef STATUSLOGGER_H
#define STATUSLOGGER_H

#include <Arduino.h>
#include <queue>
#include <string>

class StatusLogger
{
public:
    enum Output
    {
        noOutput,
        toSerial,
        toMemory
    };

    void setup(Output ot);
    void loop();
    void push(const std::string msg);
    void push(String &msg);
    void push(const char * msg);

private:
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