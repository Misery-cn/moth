#ifndef _LOG_APPENDER_H_
#define _LOG_APPENDER_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "thread.h"
#include "string_utils.h"
#include "dir_utils.h"
#include "file_utils.h"
#include "time_utils.h"

typedef enum
{
    Log_Off = 0,
    Log_Serious = 1,
    Log_Error = 2,
    Log_Info = 3,
    Log_Debug = 4
} log_level_t;

// 
const int LOG_LINE_SIZE = 512;
// 最大日志文件10M大小
const int MAX_LOG_FILE_SIZE = 10 * M;
// 最大10个日志文件
const int MAX_LOG_FILE_SEQ = 10;


class LogEvent
{
public:
	LogEvent();
	virtual ~LogEvent();

	virtual std::string format() = 0;

public:
	log_level_t _level;
	
	char _content[LOG_LINE_SIZE];
};

// 运行时日志
class RunLogEvent : public LogEvent
{
public:
	RunLogEvent(log_level_t level);
	virtual ~RunLogEvent();

	std::string format();
};


class Appender
{
public:
	Appender();
	virtual ~Appender();

	virtual void do_appender(LogEvent* logevent) = 0;
	
protected:
	// 进程名

};


class FileAppender : public Appender
{
public:
	FileAppender(const char* filename);
	virtual ~FileAppender();

	void init();

	// 检查当前日志文件,是否需要重新创建
	void check_log_file();

	void create_log_file();

	// 写文件
	void do_appender(LogEvent* logevent);

private:
	int _log_fd;
	// 当前文件序号
	int _log_file_seq;
	// 日志文件名
	char _log_file_name[FILENAME_MAX];
	// 当前日志文件名
	char _current_file_name[FILENAME_MAX];
	// 日志路径
	char _log_path[PATH_MAX];
};

#endif