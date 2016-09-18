#ifndef _SYS_RW_LOCK_H_
#define _SYS_RW_LOCK_H_

#include <pthread.h>
#include "define.h"
#include "exception.h"

// SYS_NS_BEGIN

class CRWLock
{
public:
	CRWLock() throw (CSysCallException);
	virtual ~CRWLock() throw ();

	// ½âËø
	void unlock() throw (CSysCallException);
	// ¶ÁËø
	void lock_read() throw (CSysCallException);
	// Ð´Ëø
	void lock_write() throw (CSysCallException);
	// ³¢ÊÔ¶ÁËø
	bool try_lock_read() throw (CSysCallException);
	// ³¢ÊÔÐ´Ëø
	bool try_lock_write() throw (CSysCallException);

	bool timed_lock_read(uint32_t millisecond) throw (CSysCallException);

	bool timed_lock_write(uint32_t millisecond) throw (CSysCallException);
	
private:
	pthread_rwlockattr_t attr_;
	pthread_rwlock_t rwlock_;
};

class CRWLockGuard
{
public:
	// Ä¬ÈÏ¼Ó¶ÁËø
    CRWLockGuard(CRWLock& lock, bool is_read = true) : lock_(lock)
    {
    	if (is_read)
		{
        	lock_.lock_read();
    	}
		else
		{
			lock_.lock_write();
		}
    }    
    
    ~CRWLockGuard()
    {
        lock_.unlock();
    }
    
private:
    CRWLock& lock_;
};

// SYS_NS_END

#endif