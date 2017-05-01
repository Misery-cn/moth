#ifndef _SYS_RW_LOCK_H_
#define _SYS_RW_LOCK_H_

#include <pthread.h>
#include "exception.h"

// SYS_NS_BEGIN

class RWLock
{
public:
	RWLock() throw (SysCallException);
	virtual ~RWLock() throw ();

	// ½âËø
	void unlock() throw (SysCallException);
	// ¶ÁËø
	void lock_read() throw (SysCallException);
	// Ð´Ëø
	void lock_write() throw (SysCallException);
	// ³¢ÊÔ¶ÁËø
	bool try_lock_read() throw (SysCallException);
	// ³¢ÊÔÐ´Ëø
	bool try_lock_write() throw (SysCallException);

	bool timed_lock_read(uint32_t millisecond) throw (SysCallException);

	bool timed_lock_write(uint32_t millisecond) throw (SysCallException);
	
private:
	pthread_rwlockattr_t _attr;
	pthread_rwlock_t _rwlock;
};

class RWLockGuard
{
public:
	// Ä¬ÈÏ¼Ó¶ÁËø
    RWLockGuard(RWLock& lock, bool is_read = true) : _lock(lock)
    {
    	if (is_read)
		{
        	_lock.lock_read();
    	}
		else
		{
			_lock.lock_write();
		}
    }    
    
    ~RWLockGuard()
    {
        _lock.unlock();
    }
    
private:
    RWLock& _lock;
};

// SYS_NS_END

#endif