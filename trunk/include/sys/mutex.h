#ifndef _SYS_MUTEX_H_
#define _SYS_MUTEX_H_

#include <pthread.h>
#include <sys/time.h>
#include <errno.h>
#include "exception.h"

// SYS_NS_BEGIN

class Mutex
{
	friend class Cond;
public:
	// 默认构造一个递归互斥锁
	Mutex(bool recursive = true) throw (SysCallException);
	virtual ~Mutex() throw ();
	
	// 加锁
	void lock() throw (SysCallException);
	// 解锁
	void unlock() throw (SysCallException);
	// 尝试加锁,加锁成功返回true,否则false
	bool try_lock() throw (SysCallException);
	// 如果在指定的毫秒时间内加锁成功则返回true,否则返回false
	bool timed_lock(uint32_t millisecond) throw (SysCallException);

public:
	class Locker 
	{
	public:
		explicit Locker(Mutex& m) : _mutex(m)
		{
			_mutex.lock();
		}
		
		~Locker()
		{
			_mutex.unlock();
		}
	private:
		Mutex& _mutex;
	};
	
private:
	pthread_mutexattr_t _attr;
	pthread_mutex_t _mutex;
};

class MutexGuard
{
public:
	MutexGuard(Mutex& lock) : _lock(lock)
	{
		lock.lock();
	}
	virtual ~MutexGuard()
	{
		_lock.unlock();
	}
private:
	Mutex& _lock;
};

// SYS_NS_END

#endif