#ifndef EXTENDEDSTATUSLOGGER_H
#define EXTENDEDSTATUSLOGGER_H

#include "statusLogger.hpp"

class ExtendedStatusLogger
{
public:
    ExtendedStatusLogger(std::string prefix)
    {
        unitPrefix = prefix;
        unitPrefix.append(16 - unitPrefix.length(), ' ');
    }
    void loop() { logger->loop(); }

    void push(const std::string msg)
    {
        logger->push(unitPrefix + msg);
    }

    void push(const char *msg)
    {
        std::string str{msg};
        logger->push(unitPrefix + str);
    }

    template <typename T,
              typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
    void push(T msg)
    {
        std::string str = std::to_string(msg);
        logger->push(unitPrefix + str);
    }

private:
    std::string unitPrefix;
    StatusLogger *logger = StatusLogger::getInstance();
};

#endif // EXTENDEDSTATUSLOGGER_H