#ifndef _SYS_MUTEX_H_
#define _SYS_MUTEX_H_

#include <pthread.h>
#include <sys/time.h>
#include <errno.h>
#include "exception.h"
#include "define.h"

// SYS_NS_BEGIN

class CMutex
{
	friend class CCond;
public:
	// 默认构造一个递归互斥锁
	CMutex(bool recursive = true) throw (CSysCallException);
	virtual ~CMutex() throw ();
	
	// 加锁
	void lock() throw (CSysCallException);
	// 解锁
	void unlock() throw (CSysCallException);
	// 尝试加锁,加锁成功返回true,否则false
	bool try_lock() throw (CSysCallException);
	// 如果在指定的毫秒时间内加锁成功则返回true,否则返回false
	bool timed_lock(uint32_t millisecond) throw (CSysCallException);
	
private:
	pthread_mutexattr_t attr_;
	pthread_mutex_t mutex_;
};

class CMutexGuard
{
public:
	CMutexGuard(CMutex& lock) : lock_(lock)
	{
		lock.lock();
	}
	virtual ~CMutexGuard()
	{
		lock_.unlock();
	}
private:
	CMutex& lock_;
};

// SYS_NS_END

#endif