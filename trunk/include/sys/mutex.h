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

public:
	class Locker 
	{
	public:
		explicit Locker(CMutex& m) : _mutex(m)
		{
			_mutex.lock();
		}
		
		~Locker()
		{
			_mutex.unlock();
		}
	private:
		CMutex& _mutex;
	};
	
private:
	pthread_mutexattr_t _attr;
	pthread_mutex_t _mutex;
};

class CMutexGuard
{
public:
	CMutexGuard(CMutex& lock) : _lock(lock)
	{
		lock.lock();
	}
	virtual ~CMutexGuard()
	{
		_lock.unlock();
	}
private:
	CMutex& _lock;
};

// SYS_NS_END

#endif