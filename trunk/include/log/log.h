#ifndef _LOG_H_
#define _LOG_H_

#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <stdlib.h>
#include "logger.h"
#include "singleton.h"

#define slog CSingleton<CLog>::instance()


#define RUNLOG(LEVEL, FORMAT, ...) \
	do \
	{ \
		slog.do_log(LEVEL, FORMAT, ##__VA_ARGS__);\
	} while(0)


#define DEBUG_LOG(FORMAT, ...) \
	do \
	{ \
		slog.do_log(Log_Debug, FORMAT, ##__VA_ARGS__);\
	} while(0)


#define INFO_LOG(FORMAT, ...) \
	do \
	{ \
		slog.do_log(Log_Info, FORMAT, ##__VA_ARGS__);\
	} while(0)


#endif
