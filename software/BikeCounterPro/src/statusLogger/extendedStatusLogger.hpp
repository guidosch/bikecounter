#ifndef EXTENDEDSTATUSLOGGER_H
#define EXTENDEDSTATUSLOGGER_H

#include "statusLogger.hpp"

class ExtendedStatusLogger
{
public:
    ExtendedStatusLogger(String prefix)
    {
        unitPrefix = prefix.c_str();
        unitPrefix.append(16 - unitPrefix.length(), ' ');
    }
    void loop() { logger->loop(); }

    void push(const std::string msg)
    {
        std::string str = unitPrefix + msg;
        logger->push(msg);
    }

    void push(String &msg)
    {
        std::string str{msg.c_str()};
        logger->push(unitPrefix + str);
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