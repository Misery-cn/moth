#include "logger.h"

Log::Log()
{
	init();
}

Log::~Log()
{
}

bool Log::init()
{
	// 初始化文件附加器
	Appender* appender = new FileAppender("moth");
	
	_run = new Logger();
	_run->set_appender(appender);

	return true;
}

void Log::do_log(log_level_t level, const char* format, ...)
{
	if (NULL == _run)
	{
		return;
	}

	if (level <= _run->get_log_level())
	{
		RunLogEvent event(level);
	
		va_list args;
		va_start(args, format);
		StringUtils::fix_vsnprintf(event._content, LOG_LINE_SIZE, format, args);
		va_end(args);

		_run->call_appender(&event);
	}
}

Logger::Logger()
{
	_level = Log_Info;
	// 附加器列表,目前只有一个文件附加器
	_appenders = new ArrayList<Appender*>(1);
}

Logger::~Logger()
{
}

void Logger::call_appender(LogEvent* event)
{
	if (NULL == event)
	{
		return;
	}

	_lock.lock();
	// 托管
	MutexGuard guard(_lock);

	for (uint32_t i = 0; i < _appenders->size(); i++)
	{
		Appender* ap = (*_appenders)[i];
		ap->do_appender(event);
	}
}


