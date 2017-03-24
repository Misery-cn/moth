#include "rw_lock.h"

// SYS_NS_BEGIN

CRWLock::CRWLock() throw (CSysCallException)
{
	pthread_rwlockattr_init(&_attr);
	
	int r = pthread_rwlock_init(&_rwlock, &_attr);
	if (0 != r)
	{
		pthread_rwlock_destroy(&_rwlock);
		THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_rwlock_init");
	}
}

CRWLock::~CRWLock() throw ()
{
	pthread_rwlockattr_destroy(&_attr);	
	pthread_rwlock_destroy(&_rwlock);
}

void CRWLock::unlock() throw (CSysCallException)
{
	int r = pthread_rwlock_unlock(&_rwlock);
	if (0 != r)
	{
	    THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_rwlock_unlock");
	}
}

void CRWLock::lock_read() throw (CSysCallException)
{
	int r = pthread_rwlock_rdlock(&_rwlock);
	if (0 != r)
	{
	    THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_rwlock_rdlock");
	}
}

void CRWLock::lock_write() throw (CSysCallException)
{
	int r = pthread_rwlock_wrlock(&_rwlock);
	if (0 != r)
	{
	    THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_rwlock_wrlock");
	}
}

bool CRWLock::try_lock_read() throw (CSysCallException)
{
	int r = pthread_rwlock_tryrdlock(&_rwlock);

	if (0 == r)
	{
		return true;
	}
	
	if (EBUSY == r)
	{
		return false;
	}

	THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_rwlock_tryrdlock");
}

bool CRWLock::try_lock_write() throw (CSysCallException)
{
	int r = pthread_rwlock_trywrlock(&_rwlock);
	
	if (0 == r)
	{
		return true;
	}
	
	if (EBUSY == r)
	{
		return false;
	}

	THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_rwlock_trywrlock");
}

bool CRWLock::timed_lock_read(uint32_t millisecond) throw (CSysCallException)
{
	int r = 0;

	if (0 == millisecond)
	{
	    r = pthread_rwlock_rdlock(&_rwlock);
	}
	else
	{	
		struct timespec ts;

		clock_gettime(CLOCK_REALTIME, &ts);    
		ts.tv_sec += millisecond / 1000;
		ts.tv_nsec += (millisecond % 1000) * 1000000;

		r = pthread_rwlock_timedrdlock(&_rwlock, &ts);
	}

	if (0 == r)
	{
		return true;
	}
	
	if (ETIMEDOUT == r)
	{
		return false;
	}

	THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_rwlock_timedrdlock");
}

bool CRWLock::timed_lock_write(uint32_t millisecond) throw (CSysCallException)
{
	int r = 0;

	if (0 == millisecond)
	{
	    r = pthread_rwlock_trywrlock(&_rwlock);
	}
	else
	{	
		struct timespec ts;

		clock_gettime(CLOCK_REALTIME, &ts);    
		ts.tv_sec += millisecond / 1000;
		ts.tv_nsec += (millisecond % 1000) * 1000000;

		r = pthread_rwlock_timedwrlock(&_rwlock, &ts);
	}

	if (0 == r)
	{
		return true;
	}
	
	if (ETIMEDOUT == r)
	{
		return false;
	}

	THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_rwlock_timedwrlock");
}

// SYS_NS_END