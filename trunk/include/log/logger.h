#ifndef _LOGGER_H_
#define _LOGGER_H_

#include "mutex.h"
#include "log_appender.h"


class CLogger;

// 日志类
class CLog
{
public:
	CLog();
	virtual ~CLog();

	bool init();

	void do_log(log_level_t level, const char* format, ...);

private:
	
	// 运行时日志
	CLogger* run_;
};


// 日志记录器
class CLogger
{
public:
	CLogger();
	virtual ~CLogger();

	void set_appender(CAppender* appender)
	{
		appenders_->push_back(appender);
	}

	void call_appender(CLogEvent* event);

	void set_log_level(log_level_t level)
	{
		level_ = level;
	}

	log_level_t get_log_level()
	{
		return level_;
	}

private:
	// 日志级别
	log_level_t level_;

	// 模块ID

	// 日志目的地列表
    CArrayList<CAppender*>* appenders_;

	// 共享内存

	// 线程互斥
	CMutex lock_;
};

#endif
