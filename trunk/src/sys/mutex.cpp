#include "mutex.h"

// SYS_NS_BEGIN

Mutex::Mutex(bool recursive) throw (SysCallException)
{
    int r = 0;
    // 创建递归锁
    if (recursive)
    {    
    #if defined(__linux) && !defined(__USE_UNIX98)
        _attr = PTHREAD_MUTEX_RECURSIVE_NP;
    #else
        r = pthread_mutexattr_init(&_attr);
        if (0 != r)
        {
            THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_mutexattr_init");
        }
        
        r = pthread_mutexattr_settype(&_attr, PTHREAD_MUTEX_RECURSIVE);
        if (0 != r)
        {
            THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_mutexattr_settype");
            pthread_mutexattr_destroy(&_attr);
        }
    #endif    
        r = pthread_mutex_init(&_mutex, &_attr);
    }
    else 
    {
        r = pthread_mutex_init(&_mutex, NULL);
    }
    
    if (0 != r) 
    {
        // RUNLOG or throw Exception
        pthread_mutexattr_destroy(&_attr);
        THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_mutex_init");
    }
}

Mutex::~Mutex() throw ()
{
#if defined(__linux) && !defined(__USE_UNIX98)
    //
#else
    pthread_mutexattr_destroy(&_attr);
#endif
    pthread_mutex_destroy(&_mutex);
}

void Mutex::lock() throw (SysCallException)
{
    int r = pthread_mutex_lock(&_mutex);
    if (0 != r)
    {
        // RUNLOG or throw Exception
        THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_mutex_lock");
    }
}

void Mutex::unlock() throw (SysCallException)
{
    int r = pthread_mutex_unlock(&_mutex);
    if (0 != r)
    {
        // RUNLOG or throw Exception
        THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_mutex_unlock");
    }
}

bool Mutex::try_lock() throw (SysCallException)
{
    int r = pthread_mutex_trylock(&_mutex);

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

bool Mutex::timed_lock(uint32_t millisecond) throw (SysCallException)
{
    int r;

    if (0 == millisecond)
    {
        r = pthread_mutex_lock(&_mutex);
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
        
        r = pthread_mutex_timedlock(&_mutex, &ts);
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
