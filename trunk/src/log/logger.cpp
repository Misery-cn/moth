#include "logger.h"

CLog::CLog()
{
	init();
}

CLog::~CLog()
{
}

bool CLog::init()
{
	// 初始化文件附加器
	CAppender* appender = new CFileAppender("moth");
	
	_run = new CLogger();
	_run->set_appender(appender);

	return true;
}

void CLog::do_log(log_level_t level, const char* format, ...)
{
	if (NULL == _run)
	{
		return;
	}

	if (level <= _run->get_log_level())
	{
		CRunLogEvent event(level);
	
		va_list args;
		va_start(args, format);
		CStringUtils::fix_vsnprintf(event.content_, LOG_LINE_SIZE, format, args);
		va_end(args);

		_run->call_appender(&event);
	}
}

CLogger::CLogger()
{
	_level = Log_Info;
	// 附加器列表,目前只有一个文件附加器
	_appenders = new CArrayList<CAppender*>(1);
}

CLogger::~CLogger()
{
}

void CLogger::call_appender(CLogEvent* event)
{
	if (NULL == event)
	{
		return;
	}

	_lock.lock();
	// 托管
	CMutexGuard guard(_lock);

	for (uint32_t i = 0; i < _appenders->size(); i++)
	{
		CAppender* ap = (*_appenders)[i];
		ap->do_appender(event);
	}
}


