#include "mutex.h"

// SYS_NS_BEGIN

CMutex::CMutex(bool recursive) throw (CSysCallException)
{
	int r = 0;
	// 创建递归锁
    if (recursive)
    {    
		#if defined(__linux) && !defined(__USE_UNIX98)
        attr_ = PTHREAD_MUTEX_RECURSIVE_NP;
		#else
        r = pthread_mutexattr_init(&attr_);
		if (0 != r)
		{
			THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_mutexattr_init");
		}
        
        r = pthread_mutexattr_settype(&attr_, PTHREAD_MUTEX_RECURSIVE);
        if (0 != r)
		{
			THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_mutexattr_settype");
            pthread_mutexattr_destroy(&attr_);
        }
		#endif    
        r = pthread_mutex_init(&mutex_, &attr_);
    }
    else 
	{
        r = pthread_mutex_init(&mutex_, NULL);
    }
    
    if (0 != r) 
	{
		// RUNLOG or throw Exception
        pthread_mutexattr_destroy(&attr_);
		THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_mutex_init");
    }
}

CMutex::~CMutex() throw ()
{
	#if defined(__linux) && !defined(__USE_UNIX98)
	//
	#else
    pthread_mutexattr_destroy(&attr_);
	#endif
    pthread_mutex_destroy(&mutex_);
}

void CMutex::lock() throw (CSysCallException)
{
	int r = pthread_mutex_lock(&mutex_);
    if (0 != r)
	{
		// RUNLOG or throw Exception
		THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_mutex_lock");
	}
}

void CMutex::unlock() throw (CSysCallException)
{
	int r = pthread_mutex_unlock(&mutex_);
    if (0 != r)
	{
		// RUNLOG or throw Exception
		THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_mutex_unlock");
	}
}

bool CMutex::try_lock() throw (CSysCallException)
{
	int r = pthread_mutex_trylock(&mutex_);

    if (0 == r)
	{
		return true;
	}
	
	if (EBUSY == r)
	{
		return false;
	}

    // RUNLOG or throw Exception
	THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_mutex_trylock");
}

bool CMutex::timed_lock(uint32_t millisecond) throw (CSysCallException)
{
	int r;

	if (0 == millisecond)
	{
		r = pthread_mutex_lock(&mutex_);
	}
	else
	{
		// 微秒结构
		// struct timeval tv;
		// 纳秒结构
		struct timespec ts;

#if _POSIX_C_SOURCE >= 199309L
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec += millisecond / 1000;
		ts.tv_nsec += (millisecond % 1000) * 1000000;
#else
#endif // _POSIX_C_SOURCE
        struct timeval tv;
        if (-1 == gettimeofday(&tv, NULL))
        {
			// RUNLOG or throw Exception
			THROW_SYSCALL_EXCEPTION(NULL, errno, "gettimeofday");
		}

        ts.tv_sec = tv.tv_sec;
        ts.tv_nsec = tv.tv_usec * 1000;
        ts.tv_sec += millisecond / 1000;
        ts.tv_nsec += (millisecond % 1000) * 1000000;
        
		r = pthread_mutex_timedlock(&mutex_, &ts);
	}
	
	if (0 == r)
	{
		return true;
	}
	
    if (ETIMEDOUT == r)
	{
		return false;
	}
	
    // RUNLOG or throw Exception
	THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_mutex_timedlock");
}

// SYS_NS_END
