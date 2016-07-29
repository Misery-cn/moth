#ifndef _SYS_LOG_H_
#define _SYS_LOG_H_
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
    LOG_LEVEL_DETAIL = 0,
    LOG_LEVEL_DEBUG  = 1,
    LOG_LEVEL_INFO   = 2,
    LOG_LEVEL_WARN   = 3,
    LOG_LEVEL_ERROR  = 4,
    LOG_LEVEL_FATAL  = 5,    
    LOG_LEVEL_STATE  = 6,
    LOG_LEVEL_TRACE  = 7
} LOG_LEVEL;

// 日志结构
typedef struct
{
	// 日志内容长度
    uint16_t length;
	// 日志内容
    char content[4];
} log_message_t;


namespace sys
{

// 日志基类
class CLog
{
public:
	CLog();
	virtual ~CLog(){}
	
private:

};

// 
class CLogProber
{
public:
	CLogProber();
	virtual ~CLogProber();
};


class CLogger : public CLogProber, public CLog
{
public:
	CLogger();
	~CLogger();
};

}

#endif