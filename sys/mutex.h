#include <pthread.h>
#include <sys/time.h>
#include <errno.h>

class CMutex
{
	friend class CCond;
public:
	// 默认构造一个非递归互斥锁
	CMutex(bool recursive = false);
	virtual ~CMutex();
	
	// 加锁
	void lock();
	// 解锁
	void unlock();
	// 尝试加锁,加锁成功返回true,否则false
	bool try_lock();
	// 如果在指定的毫秒时间内加锁成功则返回true,否则返回false
	bool timed_lock(unsigned int millisecond);
	
private:
	pthread_mutex_t mutex_;
	pthread_mutexattr_t attr_;
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