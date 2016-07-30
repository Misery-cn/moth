#include "rw_lock.h"

SYS_NAMESPACE_BEGIN

CRWLock::CRWLock() throw (sys::CSysCallException)
{
	pthread_rwlockattr_init(&attr_);
	
	int r = pthread_rwlock_init(&rwlock_, &attr_);
	if (0 != r)
	{
		pthread_rwlock_destroy(&rwlock_);
		THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_rwlock_init");
	}
}

CRWLock::~CRWLock() throw ()
{
	pthread_rwlockattr_destroy(&attr_);	
	pthread_rwlock_destroy(&rwlock_);
}

void CRWLock::unlock() throw (sys::CSysCallException)
{
	int r = pthread_rwlock_unlock(&rwlock_);
	if (0 != r)
	{
	    THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_rwlock_unlock");
	}
}

void CRWLock::lock_read() throw (sys::CSysCallException)
{
	int r = pthread_rwlock_rdlock(&rwlock_);
	if (0 != r)
	{
	    THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_rwlock_rdlock");
	}
}

void CRWLock::lock_write() throw (sys::CSysCallException)
{
	int r = pthread_rwlock_wrlock(&rwlock_);
	if (0 != r)
	{
	    THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_rwlock_wrlock");
	}
}

bool CRWLock::try_lock_read() throw (sys::CSysCallException)
{
	int r = pthread_rwlock_tryrdlock(&rwlock_);

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

bool CRWLock::try_lock_write() throw (sys::CSysCallException)
{
	int r = pthread_rwlock_trywrlock(&rwlock_);
	
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

bool CRWLock::timed_lock_read(uint32_t millisecond) throw (sys::CSysCallException)
{
	int r = 0;

	if (0 == millisecond)
	{
	    r = pthread_rwlock_rdlock(&rwlock_);
	}
	else
	{	
		struct timespec ts;

		clock_gettime(CLOCK_REALTIME, &ts);    
		ts.tv_sec += millisecond / 1000;
		ts.tv_nsec += (millisecond % 1000) * 1000000;

		r = pthread_rwlock_timedrdlock(&rwlock_, &ts);
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

bool CRWLock::timed_lock_write(uint32_t millisecond) throw (sys::CSysCallException)
{
	int r = 0;

	if (0 == millisecond)
	{
	    r = pthread_rwlock_trywrlock(&rwlock_);
	}
	else
	{	
		struct timespec ts;

		clock_gettime(CLOCK_REALTIME, &ts);    
		ts.tv_sec += millisecond / 1000;
		ts.tv_nsec += (millisecond % 1000) * 1000000;

		r = pthread_rwlock_timedwrlock(&rwlock_, &ts);
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

SYS_NAMESPACE_END