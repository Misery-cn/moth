#ifndef _LOGGER_H_
#define _LOGGER_H_

#include "mutex.h"
#include "log_appender.h"

class Logger;

// 日志类
class Log
{
public:
    Log();
    virtual ~Log();

    bool init();

    void do_log(log_level_t level, const char* format, ...);
    
    void set_log_level(uint32_t level);

private:
    
    // 运行时日志
    Logger* _run;
};


// 日志记录器
class Logger
{
public:
    Logger();
    virtual ~Logger();

    void set_appender(Appender* appender)
    {    
        _appenders->push_back(&appender->item);
    }

    void call_appender(LogEvent* event);

    void set_log_level(log_level_t level)
    {
        _level = level;
    }

    log_level_t get_log_level()
    {
        return _level;
    }

private:
    // 日志级别
    log_level_t _level;

    // 日志目的地列表
    ArrayList<Appender*>* _appenders;

    // 互斥锁
    Mutex _lock;
};

#endif
