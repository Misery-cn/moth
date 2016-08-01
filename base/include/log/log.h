#ifndef _LOG_H_
#define _LOG_H_

#include "define.h"
#include "const.h"

// 日志行最小长度
const int LOG_LINE_SIZE_MIN	= 256;
// 日志行最大长度(32K),最大不能超过64K,因为使用2字节无符号整数存储的
const int LOG_LINE_SIZE_MA	= 32768;
// 默认的单个日志文件大小(100MB)
const int DEFAULT_LOG_FILE_SIZE	= 100 * M;
// 默认的日志文件备份个数
const int DEFAULT_LOG_FILE_BACKUP_NUMBER = 10;

// 日志级别
typedef enum
{
	LOG_LEVEL_OFF	 = -1,
	LOG_LEVEL_DETAIL = 0,
    LOG_LEVEL_DEBUG  = 1,
    LOG_LEVEL_INFO   = 2,
    LOG_LEVEL_WARN   = 3,
    LOG_LEVEL_ERROR  = 4,
    LOG_LEVEL_FATAL  = 5,    
    LOG_LEVEL_STATE  = 6,
    LOG_LEVEL_TRACE  = 7
} log_level_t;


// 日志基类
class CLog
{
public:
	CLog() {}
    virtual ~CLog() {}

    virtual void enable_screen(bool enabled) {}

    virtual void enable_bin_log(bool enabled) {}

    virtual void enable_trace_log(bool enabled) {}

    virtual void enable_raw_log(bool enabled) {}

    virtual void enable_auto_adddot(bool enabled) {}

    virtual void enable_auto_newline(bool enabled) {}   

    virtual void set_log_level(log_level_t log_level) {}

    virtual void set_single_filesize(uint32_t filesize) {}

    virtual void set_backup_number(uint16_t backup_number) {}

    virtual bool enabled_bin() { return false; }

    virtual bool enabled_detail() { return false; }

    virtual bool enabled_debug() { return false; }
	
    virtual bool enabled_info() { return false; }

    virtual bool enabled_warn() { return false; }

    virtual bool enabled_error() { return false; }

    virtual bool enabled_fatal() { return false; }

    virtual bool enabled_state() { return false; }

    virtual bool enabled_trace() { return false; }

    virtual bool enabled_raw() { return false; }

	virtual void run_log(log_level_t level, const char* filename, int lineno, const char* module_name, const char* format, ...)  __attribute__((format(printf, 5, 6))) {}

    virtual void log_detail(const char* filename, int lineno, const char* module_name, const char* format, ...)  __attribute__((format(printf, 5, 6))) {}
    virtual void log_debug(const char* filename, int lineno, const char* module_name, const char* format, ...) __attribute__((format(printf, 5, 6)))   {}
    virtual void log_info(const char* filename, int lineno, const char* module_name, const char* format, ...) __attribute__((format(printf, 5, 6)))    {}
    virtual void log_warn(const char* filename, int lineno, const char* module_name, const char* format, ...) __attribute__((format(printf, 5, 6)))    {}
    virtual void log_error(const char* filename, int lineno, const char* module_name, const char* format, ...) __attribute__((format(printf, 5, 6)))   {}
    virtual void log_fatal(const char* filename, int lineno, const char* module_name, const char* format, ...) __attribute__((format(printf, 5, 6)))   {}
    virtual void log_state(const char* filename, int lineno, const char* module_name, const char* format, ...) __attribute__((format(printf, 5, 6)))   {}
    virtual void log_trace(const char* filename, int lineno, const char* module_name, const char* format, ...) __attribute__((format(printf, 5, 6)))   {}

    virtual void log_raw(const char* format, ...) __attribute__((format(printf, 2, 3))) {}

    virtual void log_bin(const char* filename, int lineno, const char* module_name, const char* log, uint16_t size) {}
};

extern log_level_t get_log_level(const char* level_name);

extern const char* get_log_level_name(log_level_t log_level);

extern std::string get_log_filename();

extern std::string get_log_dirpath(bool enable_program_path = true);

extern std::string get_log_filepath(bool enable_program_path = true);

extern void set_log_level(CLog* logger);

extern void enable_screen_log(CLog* logger);



extern CLog* g_logger;

#define __MYLOG_DETAIL(logger, module_name, format, ...) \
do { \
	if (NULL == logger) { \
	    printf("[DETAIL][%s:%d]", __FILE__, __LINE__); \
		printf(format, ##__VA_ARGS__); \
	} \
	else if (logger->enabled_detail()) { \
		logger->log_detail(__FILE__, __LINE__, module_name, format, ##__VA_ARGS__); \
	} \
} while(false)

#define __MYLOG_DEBUG(logger, module_name, format, ...) \
do { \
	if (NULL == logger) { \
	    printf(PRINT_COLOR_DARY_GRAY"[DEBUG][%s:%d]"PRINT_COLOR_NONE, __FILE__, __LINE__); \
		printf(format, ##__VA_ARGS__); \
	} \
	else if (logger->enabled_debug()) { \
		logger->log_debug(__FILE__, __LINE__, module_name, format, ##__VA_ARGS__); \
	} \
} while(false)

#define __MYLOG_INFO(logger, module_name, format, ...) \
do { \
	if (NULL == logger) { \
	    printf("[INFO][%s:%d]", __FILE__, __LINE__); \
		printf(format, ##__VA_ARGS__); \
	} \
	else if (logger->enabled_info()) { \
		logger->log_info(__FILE__, __LINE__, module_name, format, ##__VA_ARGS__); \
	} \
} while(false)

#define __MYLOG_WARN(logger, module_name, format, ...) \
do { \
	if (NULL == logger) { \
	    printf(PRINT_COLOR_YELLOW"[WARN][%s:%d]"PRINT_COLOR_NONE, __FILE__, __LINE__); \
		printf(format, ##__VA_ARGS__); \
	} \
	else if (logger->enabled_warn()) { \
		logger->log_warn(__FILE__, __LINE__, module_name, format, ##__VA_ARGS__); \
	} \
} while(false)

#define __MYLOG_ERROR(logger, module_name, format, ...) \
do { \
	if (NULL == logger) { \
	    printf(PRINT_COLOR_RED"[ERROR][%s:%d]"PRINT_COLOR_NONE, __FILE__, __LINE__); \
		printf(format, ##__VA_ARGS__); \
	} \
	else if (logger->enabled_error()) { \
		logger->log_error(__FILE__, __LINE__, module_name, format, ##__VA_ARGS__); \
	} \
} while(false)

#define __MYLOG_FATAL(logger, module_name, format, ...) \
do { \
	if (NULL == logger) { \
	    printf(PRINT_COLOR_BROWN"[FATAL][%s:%d]"PRINT_COLOR_NONE, __FILE__, __LINE__); \
		printf(format, ##__VA_ARGS__); \
	} \
	else if (logger->enabled_fatal()) { \
		logger->log_fatal(__FILE__, __LINE__, module_name, format, ##__VA_ARGS__); \
	} \
} while(false)

#define __MYLOG_STATE(logger, module_name, format, ...) \
do { \
	if (NULL == logger) { \
	    printf("[STATE][%s:%d]", __FILE__, __LINE__); \
		printf(format, ##__VA_ARGS__); \
	} \
	else if (logger->enabled_state()) { \
		logger->log_state(__FILE__, __LINE__, module_name, format, ##__VA_ARGS__); \
	} \
} while(false)

#define __MYLOG_TRACE(logger, module_name, format, ...) \
do { \
	if (NULL == logger) { \
	    printf("[TRACE][%s:%d]", __FILE__, __LINE__); \
		printf(format, ##__VA_ARGS__); \
	} \
	else if (logger->enabled_trace()) { \
		logger->log_trace(__FILE__, __LINE__, module_name, format, ##__VA_ARGS__); \
	} \
} while(false)

#define __MYLOG_RAW(logger, format, ...) \
do { \
    if (NULL == logger) { \
        printf(format, ##__VA_ARGS__); \
    } \
    else if (logger->enabled_raw()) { \
        logger->log_raw(format, ##__VA_ARGS__); \
    } \
} while(false)

#define __MYLOG_BIN(logger, module_name, log, size) \
do { \
    if ((logger != NULL) && logger->enabled_bin()) \
        logger->log_bin(__FILE__, __LINE__, module_name, log, size); \
} while(false)

#define MYLOG_BIN(log, size)         __MYLOG_BIN(g_logger, log, size)
#define MYLOG_RAW(format, ...)     __MYLOG_RAW(g_logger, format, ##__VA_ARGS__)
#define MYLOG_TRACE(format, ...)     __MYLOG_TRACE(g_logger, NULL, format, ##__VA_ARGS__)
#define MYLOG_STATE(format, ...)     __MYLOG_STATE(g_logger, NULL, format, ##__VA_ARGS__)
#define MYLOG_FATAL(format, ...)     __MYLOG_FATAL(g_logger, NULL, format, ##__VA_ARGS__)
#define MYLOG_ERROR(format, ...)     __MYLOG_ERROR(g_logger, NULL, format, ##__VA_ARGS__)
#define MYLOG_WARN(format, ...)      __MYLOG_WARN(g_logger, NULL, format, ##__VA_ARGS__)
#define MYLOG_INFO(format, ...)      __MYLOG_INFO(g_logger, NULL, format, ##__VA_ARGS__)
#define MYLOG_DEBUG(format, ...)     __MYLOG_DEBUG(g_logger, NULL, format, ##__VA_ARGS__)
#define MYLOG_DETAIL(format, ...)    __MYLOG_DETAIL(g_logger, NULL, format, ##__VA_ARGS__)

#endif